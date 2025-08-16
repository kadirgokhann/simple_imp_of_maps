#include <vector>
#include <functional>
#include <utility>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

template<
    class Key,
    class T,
    class Hash = std::hash<Key>,
    class KeyEq = std::equal_to<Key>
>
class flat_unordered_map {
public:
    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = std::pair<const Key, T>;
    using size_type       = std::size_t;

private:
    enum class State : uint8_t { Empty, Filled, Deleted };

    struct Bucket {
        Key   key;
        T     value;
        State state = State::Empty;

        Bucket() : key(), value(), state(State::Empty) {}
    };

    std::vector<Bucket> buckets_;
    size_type           size_ = 0;           // # of Filled buckets
    size_type           tombstones_ = 0;     // # of Deleted buckets
    float               max_load_factor_ = 0.7f;

    Hash  hasher_;
    KeyEq keyeq_;

    static size_type next_pow2(size_type x) {
        if (x < 2) return 2;
        --x;
        for (size_type i = 1; i < sizeof(size_type) * 8; i <<= 1) x |= x >> i;
        return x + 1;
    }

    size_type mask() const { return buckets_.size() - 1; }

    size_type bucket_index(const Key& k) const {
        return hasher_(k) & mask(); // requires capacity power-of-two
    }

    float current_load() const {
        return static_cast<float>(size_ + tombstones_) / static_cast<float>(buckets_.size());
    }

    void rehash_if_needed() {
        if (buckets_.empty() || current_load() > max_load_factor_) {
            size_type new_cap = buckets_.empty() ? 16 : buckets_.size() * 2;
            rehash(new_cap);
        }
    }

    // Core insertion helper: insert-or-assign
    template <class K, class V>
    std::pair<size_type, bool> insert_or_assign_impl(K&& k, V&& v) {
        rehash_if_needed();

        size_type idx = bucket_index(k);
        size_type first_deleted = static_cast<size_type>(-1);

        for (;;) {
            Bucket& b = buckets_[idx];
            if (b.state == State::Empty) {
                // Use earlier deleted slot if found
                size_type target = (first_deleted == static_cast<size_type>(-1)) ? idx : first_deleted;
                Bucket& t = buckets_[target];

                if (t.state == State::Deleted) tombstones_--;
                t.key   = std::forward<K>(k);
                t.value = std::forward<V>(v);
                t.state = State::Filled;
                size_++;
                return {target, true};
            } else if (b.state == State::Deleted) {
                if (first_deleted == static_cast<size_type>(-1)) first_deleted = idx;
            } else { // Filled
                if (keyeq_(b.key, k)) {
                    b.value = std::forward<V>(v); // assign
                    return {idx, false};
                }
            }
            idx = (idx + 1) & mask();
        }
    }

public:
    flat_unordered_map() = default;

    explicit flat_unordered_map(size_type bucket_count,
                                const Hash& h = Hash(),
                                const KeyEq& eq = KeyEq())
        : size_(0), tombstones_(0), hasher_(h), keyeq_(eq) {
        bucket_count = next_pow2(bucket_count);
        buckets_.resize(bucket_count);
        // All buckets default to Empty
    }

    void rehash(size_type new_bucket_count) {
        new_bucket_count = next_pow2(new_bucket_count);
        if (new_bucket_count < 2) new_bucket_count = 2;

        std::vector<Bucket> old = std::move(buckets_);
        buckets_.assign(new_bucket_count, Bucket{});
        size_ = 0;
        tombstones_ = 0;

        for (auto& b : old) {
            if (b.state == State::Filled) {
                insert_or_assign_impl(b.key, b.value);
            }
        }
    }

    // insert or assign
    std::pair<bool, T*> insert_or_assign(const Key& k, const T& v) {
        auto [pos, inserted] = insert_or_assign_impl(k, v);
        return {inserted, &buckets_[pos].value};
    }
    std::pair<bool, T*> insert_or_assign(Key&& k, T&& v) {
        auto [pos, inserted] = insert_or_assign_impl(std::move(k), std::move(v));
        return {inserted, &buckets_[pos].value};
    }

    // find -> pointer to value (nullptr if not found)
    T* find(const Key& k) {
        if (buckets_.empty()) return nullptr;
        size_type idx = bucket_index(k);
        for (;;) {
            Bucket& b = buckets_[idx];
            if (b.state == State::Empty) return nullptr; // stop on Empty
            if (b.state == State::Filled && keyeq_(b.key, k)) return &b.value;
            idx = (idx + 1) & mask();
        }
    }

    const T* find(const Key& k) const {
        if (buckets_.empty()) return nullptr;
        size_type idx = bucket_index(k);
        for (;;) {
            const Bucket& b = buckets_[idx];
            if (b.state == State::Empty) return nullptr;
            if (b.state == State::Filled && keyeq_(b.key, k)) return &b.value;
            idx = (idx + 1) & mask();
        }
    }

    // operator[] inserts default if missing
    T& operator[](const Key& k) {
        if (auto* p = find(k)) return *p;
        auto [inserted, ptr] = insert_or_assign(k, T{});
        (void)inserted;
        return *ptr;
    }

    T& operator[](Key&& k) {
        if (auto* p = find(k)) return *p;
        auto [inserted, ptr] = insert_or_assign(std::move(k), T{});
        (void)inserted;
        return *ptr;
    }

    // erase -> true if erased
    bool erase(const Key& k) {
        if (buckets_.empty()) return false;
        size_type idx = bucket_index(k);
        for (;;) {
            Bucket& b = buckets_[idx];
            if (b.state == State::Empty) return false; // not found
            if (b.state == State::Filled && keyeq_(b.key, k)) {
                b.state = State::Deleted;
                // value/key remain but are logically removed
                size_--;
                tombstones_++;
                // Optional: shrink/rehash if many tombstones
                if (tombstones_ > buckets_.size() / 2) rehash(buckets_.size());
                return true;
            }
            idx = (idx + 1) & mask();
        }
    }

    void clear() {
        for (auto& b : buckets_) b.state = State::Empty;
        size_ = 0;
        tombstones_ = 0;
    }

    size_type size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_type bucket_count() const { return buckets_.size(); }

    void reserve(size_type n) {
        // reserve so that load after n inserts stays under max_load_factor_
        size_type needed = static_cast<size_type>(n / max_load_factor_) + 1;
        if (needed > buckets_.size()) rehash(needed);
    }

    void max_load_factor(float f) {
        if (f <= 0.1f || f >= 0.95f) throw std::invalid_argument("unreasonable load factor");
        max_load_factor_ = f;
        rehash_if_needed();
    }

    float max_load_factor() const { return max_load_factor_; }
};


#include <iostream>
#include <string>

int main() {
    flat_unordered_map<int, std::string> fm;
    fm.reserve(8);

    fm.insert_or_assign(1, "apple");
    fm.insert_or_assign(2, "banana");
    fm[3] = "cherry";          // inserts default then assigns

    if (auto* v = fm.find(2)) {
        std::cout << "2 -> " << *v << "\n";
    }

    fm.erase(1);
    std::cout << "size=" << fm.size() << ", buckets=" << fm.bucket_count() << "\n";
}

