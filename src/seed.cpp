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
)sql";

}  // namespace

void seedDatabase(Database& db) {
    db.execute(kSeedSql);
}

}  // namespace negiysem
