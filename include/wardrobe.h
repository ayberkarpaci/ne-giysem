#pragma once

#include <optional>
#include <string>
#include <vector>

namespace negiysem {

class Database;

struct WardrobeValue {
    std::string attribute_slug;
    std::string attribute_name;
    std::string value_slug;
    std::string value_name;
};

struct WardrobeItem {
    int id = 0;
    std::string type_slug;
    std::string type_name;
    std::string category_slug;
    std::string category_name;
    std::string label;        // user-given name, may be empty
    std::string photo_path;   // file name under data/photos/, may be empty
    std::string cutout_path;  // file name under data/photos/cutouts/, may be empty
    std::vector<WardrobeValue> values;
};

struct AttributeOption {
    std::string slug;
    std::string name;
};

struct AttributeDef {
    std::string slug;
    std::string name;
    std::vector<AttributeOption> values;
};

struct TypeOption {
    std::string slug;
    std::string name;
    std::string category_slug;
    std::string category_name;
};

// CRUD for the user's own garments on top of the shared Database.
class WardrobeRepository {
public:
    explicit WardrobeRepository(Database& db) : db_(db) {}

    // Returns the new item id. Throws std::runtime_error on unknown type or
    // value slugs (the whole insert is rolled back).
    int addItem(const std::string& type_slug,
                const std::string& label,
                const std::vector<std::string>& value_slugs);

    bool removeItem(int id);
    // Renames the item; returns false when the id is unknown.
    bool setLabel(int id, const std::string& label);
    // Replacing the photo also clears the cutout, which was made from the
    // old photo; the caller removes the stale file.
    void setPhotoPath(int id, const std::string& file_name);
    std::optional<std::string> photoPath(int id) const;
    void setCutoutPath(int id, const std::string& file_name);
    std::optional<std::string> cutoutPath(int id) const;

    std::vector<WardrobeItem> listItems(const std::string& lang) const;
    int count() const;

    // Garment types the user can pick from (the clothing_items catalog).
    std::vector<TypeOption> listTypes(const std::string& lang) const;

    // Attributes (with allowed values) that apply to the given type's category.
    std::vector<AttributeDef> attributesForType(const std::string& type_slug,
                                                const std::string& lang) const;

private:
    Database& db_;
};

}  // namespace negiysem
