#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

struct Entity {
    int id;
    std::string name;
    float health;
};

class EntityManager {
    std::vector<Entity> entities;                  // contiguous storage
    std::unordered_map<int, size_t> indexOf;       // maps entity ID -> index in vector

public:
    // Insert new entity
    void addEntity(int id, std::string name, float health) {
        if (indexOf.find(id) != indexOf.end()) {
            std::cout << "Entity " << id << " already exists!\n";
            return;
        }
        size_t index = entities.size();
        entities.push_back({id, std::move(name), health});
        indexOf[id] = index;
    }

    // Find entity by ID
    Entity* getEntity(int id) {
        auto it = indexOf.find(id);
        if (it == indexOf.end()) return nullptr;
        return &entities[it->second];
    }

    // Remove entity (swap-and-pop to keep vector packed)
    void removeEntity(int id) {
        auto it = indexOf.find(id);
        if (it == indexOf.end()) return;

        size_t index = it->second;
        size_t lastIndex = entities.size() - 1;

        if (index != lastIndex) {
            // Move last entity into removed slot
            entities[index] = std::move(entities[lastIndex]);
            indexOf[entities[index].id] = index;
        }

        entities.pop_back();
        indexOf.erase(it);
    }

    // Iterate over all entities (perfect locality!)
    void printAll() const {
        for (auto const& e : entities) {
            std::cout << "Entity " << e.id << " (" << e.name
                      << ") health=" << e.health << "\n";
        }
    }
};

// ---------------- Example Usage ----------------
int main() {
    EntityManager mgr;
    mgr.addEntity(1, "Orc", 100.f);
    mgr.addEntity(2, "Elf", 80.f);
    mgr.addEntity(3, "Dwarf", 120.f);

    mgr.printAll();

    std::cout << "\nRemoving entity 2...\n";
    mgr.removeEntity(2);

    mgr.printAll();

    if (auto* e = mgr.getEntity(3)) {
        std::cout << "Found entity " << e->id << " with health " << e->health << "\n";
    }
}
