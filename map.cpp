#include <iostream>
#include <utility>

template <typename Key, typename Value>
class SimpleMap {
    struct Node {
        Key key;
        Value value;
        Node* left;
        Node* right;
        Node(const Key& k, const Value& v)
            : key(k), value(v), left(nullptr), right(nullptr) {}
    };

    Node* root = nullptr;

    // recursive insert
    Node* insert(Node* node, const Key& key, const Value& value) {
        if (!node) return new Node(key, value);

        if (key < node->key) {
            node->left = insert(node->left, key, value);
        } else if (key > node->key) {
            node->right = insert(node->right, key, value);
        } else {
            node->value = value; // update existing key
        }
        return node;
    }

    // recursive find
    Node* find(Node* node, const Key& key) const {
        if (!node) return nullptr;
        if (key < node->key) return find(node->left, key);
        if (key > node->key) return find(node->right, key);
        return node; // found
    }

    // recursive cleanup
    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    void inorder(Node* node) const {
        if (!node) return;
        inorder(node->left);
        std::cout << node->key << " => " << node->value << "\n";
        inorder(node->right);
    }

public:
    ~SimpleMap() { destroy(root); }

    void insert(const Key& key, const Value& value) {
        root = insert(root, key, value);
    }

    Value* find(const Key& key) {
        Node* n = find(root, key);
        return n ? &n->value : nullptr;
    }

    void printInOrder() const {
        inorder(root);
    }
};

// ---------------- Example ----------------
int main() {
    SimpleMap<int, std::string> map;
    map.insert(5, "apple");
    map.insert(3, "banana");
    map.insert(7, "cherry");
    map.insert(2, "pear");

    map.printInOrder();  // prints keys in sorted order

    if (auto val = map.find(3)) {
        std::cout << "Found key 3: " << *val << "\n";
    } else {
        std::cout << "Key 3 not found\n";
    }

    return 0;
}
