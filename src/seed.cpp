#include "seed.h"

#include "database.h"

namespace negiysem {

namespace {

// Fixed IDs keep the affinity/translation inserts readable and idempotent.
constexpr const char* kSeedSql = R"sql(
INSERT OR IGNORE INTO clothing_categories (id, slug) VALUES
    (1, 'outerwear'),
    (2, 'top'),
    (3, 'bottom'),
    (4, 'footwear'),
    (5, 'accessory');

INSERT OR IGNORE INTO clothing_items
    (id, category_id, slug, min_temp_c, max_temp_c, is_waterproof, warmth_level) VALUES
    -- tops
    (1,  2, 't-shirt',        18,  45, 0, 0),
    (2,  2, 'shirt',          15,  28, 0, 1),
    (3,  2, 'hoodie',          8,  18, 0, 3),
    (4,  2, 'sweater',        -5,  14, 0, 4),
    -- bottoms
    (5,  3, 'shorts',         22,  45, 0, 0),
    (6,  3, 'jeans',           5,  24, 0, 2),
    (7,  3, 'chinos',         10,  26, 0, 1),
    (8,  3, 'sweatpants',     -5,  16, 0, 3),
    -- footwear
    (9,  4, 'sandals',        24,  45, 0, 0),
    (10, 4, 'sneakers',        8,  30, 0, 1),
    (11, 4, 'boots',         -15,  12, 1, 4),
    -- outerwear
    (12, 1, 'raincoat',        5,  20, 1, 2),
    (13, 1, 'puffer-jacket', -20,   8, 0, 5),
    (14, 1, 'denim-jacket',   12,  20, 0, 2),
    (15, 1, 'trench-coat',     8,  16, 0, 3),
    -- accessories
    (16, 5, 'umbrella',      NULL, NULL, 1, 0),
    (17, 5, 'beanie',        -20,   8, 0, 3),
    (18, 5, 'sunglasses',     18,  45, 0, 0),
    (19, 5, 'scarf',         -15,  10, 0, 3);

INSERT OR IGNORE INTO moods (id, slug) VALUES
    (1, 'energetic'),
    (2, 'cozy'),
    (3, 'confident'),
    (4, 'relaxed'),
    (5, 'adventurous');

INSERT OR IGNORE INTO item_mood_affinity (item_id, mood_id, weight) VALUES
    (1, 1, 0.6), (1, 4, 0.5),               -- t-shirt: energetic, relaxed
    (2, 3, 0.9),                            -- shirt: confident
    (3, 2, 0.9),                            -- hoodie: cozy
    (4, 2, 0.8),                            -- sweater: cozy
    (5, 1, 0.7),                            -- shorts: energetic
    (6, 4, 0.6),                            -- jeans: relaxed
    (7, 3, 0.8),                            -- chinos: confident
    (8, 2, 0.9),                            -- sweatpants: cozy
    (10, 1, 0.8), (10, 4, 0.5),             -- sneakers: energetic, relaxed
    (11, 5, 0.8),                           -- boots: adventurous
    (12, 5, 0.3),                           -- raincoat: adventurous
    (14, 5, 0.6),                           -- denim-jacket: adventurous
    (15, 3, 0.9),                           -- trench-coat: confident
    (17, 2, 0.6),                           -- beanie: cozy
    (18, 3, 0.5),                           -- sunglasses: confident
    (19, 2, 0.7);                           -- scarf: cozy

INSERT OR IGNORE INTO translations (entity_type, entity_id, lang_code, name) VALUES
    ('category', 1, 'en', 'Outerwear'),     ('category', 1, 'tr', 'Dış giyim'),
    ('category', 2, 'en', 'Top'),           ('category', 2, 'tr', 'Üst giyim'),
    ('category', 3, 'en', 'Bottom'),        ('category', 3, 'tr', 'Alt giyim'),
    ('category', 4, 'en', 'Footwear'),      ('category', 4, 'tr', 'Ayakkabı'),
    ('category', 5, 'en', 'Accessory'),     ('category', 5, 'tr', 'Aksesuar'),

    ('item', 1,  'en', 'T-shirt'),          ('item', 1,  'tr', 'Tişört'),
    ('item', 2,  'en', 'Shirt'),            ('item', 2,  'tr', 'Gömlek'),
    ('item', 3,  'en', 'Hoodie'),           ('item', 3,  'tr', 'Kapüşonlu sweatshirt'),
    ('item', 4,  'en', 'Sweater'),          ('item', 4,  'tr', 'Kazak'),
    ('item', 5,  'en', 'Shorts'),           ('item', 5,  'tr', 'Şort'),
    ('item', 6,  'en', 'Jeans'),            ('item', 6,  'tr', 'Kot pantolon'),
    ('item', 7,  'en', 'Chinos'),           ('item', 7,  'tr', 'Chino pantolon'),
    ('item', 8,  'en', 'Sweatpants'),       ('item', 8,  'tr', 'Eşofman altı'),
    ('item', 9,  'en', 'Sandals'),          ('item', 9,  'tr', 'Sandalet'),
    ('item', 10, 'en', 'Sneakers'),         ('item', 10, 'tr', 'Spor ayakkabı'),
    ('item', 11, 'en', 'Boots'),            ('item', 11, 'tr', 'Bot'),
    ('item', 12, 'en', 'Raincoat'),         ('item', 12, 'tr', 'Yağmurluk'),
    ('item', 13, 'en', 'Puffer jacket'),    ('item', 13, 'tr', 'Şişme mont'),
    ('item', 14, 'en', 'Denim jacket'),     ('item', 14, 'tr', 'Kot ceket'),
    ('item', 15, 'en', 'Trench coat'),      ('item', 15, 'tr', 'Trençkot'),
    ('item', 16, 'en', 'Umbrella'),         ('item', 16, 'tr', 'Şemsiye'),
    ('item', 17, 'en', 'Beanie'),           ('item', 17, 'tr', 'Bere'),
    ('item', 18, 'en', 'Sunglasses'),       ('item', 18, 'tr', 'Güneş gözlüğü'),
    ('item', 19, 'en', 'Scarf'),            ('item', 19, 'tr', 'Atkı'),

    ('mood', 1, 'en', 'Energetic'),         ('mood', 1, 'tr', 'Enerjik'),
    ('mood', 2, 'en', 'Cozy'),              ('mood', 2, 'tr', 'Keyifli'),
    ('mood', 3, 'en', 'Confident'),         ('mood', 3, 'tr', 'Özgüvenli'),
    ('mood', 4, 'en', 'Relaxed'),           ('mood', 4, 'tr', 'Sakin'),
    ('mood', 5, 'en', 'Adventurous'),       ('mood', 5, 'tr', 'Maceracı');

INSERT OR IGNORE INTO attributes (id, slug) VALUES
    (1, 'color'),
    (2, 'material'),
    (3, 'collar'),
    (4, 'fit');

INSERT OR IGNORE INTO category_attributes (category_id, attribute_id) VALUES
    -- color applies everywhere
    (1, 1), (2, 1), (3, 1), (4, 1), (5, 1),
    -- material for everything except accessories
    (1, 2), (2, 2), (3, 2), (4, 2),
    -- collar only for tops, fit only for bottoms
    (2, 3),
    (3, 4);

INSERT OR IGNORE INTO attribute_values (id, attribute_id, slug) VALUES
    (1,  1, 'black'),   (2,  1, 'white'),  (3,  1, 'gray'),
    (4,  1, 'navy'),    (5,  1, 'blue'),   (6,  1, 'red'),
    (7,  1, 'green'),   (8,  1, 'beige'),  (9,  1, 'brown'),
    (10, 1, 'yellow'),  (11, 1, 'pink'),   (12, 1, 'purple'),
    (13, 2, 'cotton'),  (14, 2, 'linen'),  (15, 2, 'wool'),
    (16, 2, 'denim'),   (17, 2, 'leather'),(18, 2, 'synthetic'),
    (19, 3, 'crew-neck'), (20, 3, 'v-neck'), (21, 3, 'polo-collar'), (22, 3, 'turtleneck'),
    (23, 4, 'slim-fit'),  (24, 4, 'regular-fit'), (25, 4, 'wide-leg');

INSERT OR IGNORE INTO translations (entity_type, entity_id, lang_code, name) VALUES
    ('attribute', 1, 'en', 'Color'),        ('attribute', 1, 'tr', 'Renk'),
    ('attribute', 2, 'en', 'Material'),     ('attribute', 2, 'tr', 'Kumaş'),
    ('attribute', 3, 'en', 'Collar'),       ('attribute', 3, 'tr', 'Yaka'),
    ('attribute', 4, 'en', 'Fit'),          ('attribute', 4, 'tr', 'Kesim'),

    ('attribute_value', 1,  'en', 'Black'),      ('attribute_value', 1,  'tr', 'Siyah'),
    ('attribute_value', 2,  'en', 'White'),      ('attribute_value', 2,  'tr', 'Beyaz'),
    ('attribute_value', 3,  'en', 'Gray'),       ('attribute_value', 3,  'tr', 'Gri'),
    ('attribute_value', 4,  'en', 'Navy'),       ('attribute_value', 4,  'tr', 'Lacivert'),
    ('attribute_value', 5,  'en', 'Blue'),       ('attribute_value', 5,  'tr', 'Mavi'),
    ('attribute_value', 6,  'en', 'Red'),        ('attribute_value', 6,  'tr', 'Kırmızı'),
    ('attribute_value', 7,  'en', 'Green'),      ('attribute_value', 7,  'tr', 'Yeşil'),
    ('attribute_value', 8,  'en', 'Beige'),      ('attribute_value', 8,  'tr', 'Bej'),
    ('attribute_value', 9,  'en', 'Brown'),      ('attribute_value', 9,  'tr', 'Kahverengi'),
    ('attribute_value', 10, 'en', 'Yellow'),     ('attribute_value', 10, 'tr', 'Sarı'),
    ('attribute_value', 11, 'en', 'Pink'),       ('attribute_value', 11, 'tr', 'Pembe'),
    ('attribute_value', 12, 'en', 'Purple'),     ('attribute_value', 12, 'tr', 'Mor'),
    ('attribute_value', 13, 'en', 'Cotton'),     ('attribute_value', 13, 'tr', 'Pamuk'),
    ('attribute_value', 14, 'en', 'Linen'),      ('attribute_value', 14, 'tr', 'Keten'),
    ('attribute_value', 15, 'en', 'Wool'),       ('attribute_value', 15, 'tr', 'Yün'),
    ('attribute_value', 16, 'en', 'Denim'),      ('attribute_value', 16, 'tr', 'Kot'),
    ('attribute_value', 17, 'en', 'Leather'),    ('attribute_value', 17, 'tr', 'Deri'),
    ('attribute_value', 18, 'en', 'Synthetic'),  ('attribute_value', 18, 'tr', 'Sentetik'),
    ('attribute_value', 19, 'en', 'Crew neck'),  ('attribute_value', 19, 'tr', 'Bisiklet yaka'),
    ('attribute_value', 20, 'en', 'V-neck'),     ('attribute_value', 20, 'tr', 'V yaka'),
    ('attribute_value', 21, 'en', 'Polo collar'),('attribute_value', 21, 'tr', 'Polo yaka'),
    ('attribute_value', 22, 'en', 'Turtleneck'), ('attribute_value', 22, 'tr', 'Balıkçı yaka'),
    ('attribute_value', 23, 'en', 'Slim fit'),   ('attribute_value', 23, 'tr', 'Dar kesim'),
    ('attribute_value', 24, 'en', 'Regular fit'),('attribute_value', 24, 'tr', 'Normal kesim'),
    ('attribute_value', 25, 'en', 'Wide leg'),   ('attribute_value', 25, 'tr', 'Bol paça');
)sql";

}  // namespace

void seedDatabase(Database& db) {
    db.execute(kSeedSql);
}

}  // namespace negiysem
