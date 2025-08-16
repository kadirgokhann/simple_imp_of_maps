#include <iostream>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <string>

class UnorderedMap {
public:
    using Key   = int;
    using Value = std::string;
    using Pair  = std::pair<Key, Value>;

private:
    static const size_t DEFAULT_BUCKETS = 8;

    std::vector<std::list<Pair>> buckets;
    size_t numElements = 0;

    size_t getBucketIndex(const Key& key) const {
        return std::hash<Key>{}(key) % buckets.size();
    }

public:
    UnorderedMap(size_t bucketCount = DEFAULT_BUCKETS)
        : buckets(bucketCount) {}

    // Insert or update
    void insert(const Key& key, const Value& value) {
        size_t idx = getBucketIndex(key);
        for (auto& kv : buckets[idx]) {
            if (kv.first == key) {
                kv.second = value;  // update
                return;
            }
        }
        buckets[idx].push_back({key, value});  // new insert
        numElements++;
    }

    // Lookup
    Value* find(const Key& key) {
        size_t idx = getBucketIndex(key);
        for (auto& kv : buckets[idx]) {
            if (kv.first == key) {
                return &kv.second;
            }
        }
        return nullptr;  // not found
    }

    // Operator[]
    Value& operator[](const Key& key) {
        size_t idx = getBucketIndex(key);
        for (auto& kv : buckets[idx]) {
            if (kv.first == key) {
                return kv.second;
            }
        }
        // if not found, insert default value
        buckets[idx].push_back({key, Value{}});
        numElements++;
        return buckets[idx].back().second;
    }

    void erase(const Key& key) {
        size_t idx = getBucketIndex(key);
        for (auto it = buckets[idx].begin(); it != buckets[idx].end(); ++it) {
            if (it->first == key) {
                buckets[idx].erase(it);
                numElements--;
                return;
            }
        }
    }

    size_t size() const {
        return numElements;
    }
};

int main() {
    UnorderedMap map;

    map.insert(1, "apple");
    map.insert(2, "banana");

    // Operator[]
    map[3] = "cherry";

    if (auto val = map.find(2)) {
        std::cout << "Found: " << *val << "\n";
    }

    std::cout << "Map size = " << map.size() << "\n";

    map.erase(1);
    std::cout << "Map size after erase = " << map.size() << "\n";

    return 0;
}
