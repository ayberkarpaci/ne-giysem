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
    (5, 'accessory'),
    (6, 'jewelry');

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
    (19, 5, 'scarf',         -15,  10, 0, 3),
    -- more tops
    (20, 2, 'jersey',         12,  35, 0, 0),
    (21, 2, 'polo-shirt',     16,  30, 0, 0),
    (22, 2, 'cardigan',        6,  18, 0, 3),
    -- more bottoms
    (23, 3, 'leggings',        5,  22, 0, 1),
    (24, 3, 'cargo-pants',     8,  26, 0, 2),
    -- more footwear
    (25, 4, 'loafers',        12,  30, 0, 0),
    -- more outerwear
    (26, 1, 'blazer',         12,  24, 0, 1),
    (27, 1, 'wool-coat',     -10,  10, 0, 4),
    -- more accessories
    (28, 5, 'cap',            15,  45, 0, 0),
    (29, 5, 'gloves',        -20,   5, 0, 4),
    -- expanded outerwear (e-commerce style coverage)
    (30, 1, 'leather-jacket',  8,  18, 0, 3),
    (31, 1, 'bomber-jacket',   8,  18, 0, 3),
    (32, 1, 'parka',         -15,   8, 0, 5),
    (33, 1, 'windbreaker',    10,  20, 0, 2),
    (34, 1, 'puffer-vest',     5,  15, 0, 3),
    (35, 1, 'overcoat',       -5,  10, 0, 4),
    -- expanded tops
    (36, 2, 'sweatshirt',      8,  18, 0, 3),
    (37, 2, 'tank-top',       24,  45, 0, 0),
    (38, 2, 'blouse',         16,  28, 0, 0),
    (39, 2, 'flannel-shirt',   5,  16, 0, 2),
    -- expanded bottoms
    (40, 3, 'dress-pants',     8,  26, 0, 1),
    (41, 3, 'skirt',          16,  35, 0, 0),
    (42, 3, 'linen-pants',    20,  40, 0, 0),
    -- expanded footwear
    (43, 4, 'heels',          10,  35, 0, 0),
    (44, 4, 'flats',          12,  32, 0, 0),
    (45, 4, 'classic-shoes',   0,  30, 0, 1),
    -- expanded accessories
    (46, 5, 'belt',          NULL, NULL, 0, 0),
    (47, 5, 'tie',           NULL, NULL, 0, 0),
    (48, 5, 'backpack',      NULL, NULL, 0, 0),
    (49, 5, 'handbag',       NULL, NULL, 0, 0),
    -- jewelry (Swarovski-style categories); weather-neutral
    (50, 6, 'necklace',      NULL, NULL, 0, 0),
    (51, 6, 'earrings',      NULL, NULL, 0, 0),
    (52, 6, 'bracelet',      NULL, NULL, 0, 0),
    (53, 6, 'ring',          NULL, NULL, 0, 0),
    (54, 6, 'watch',         NULL, NULL, 0, 0),
    (55, 6, 'brooch',        NULL, NULL, 0, 0);

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
    (19, 2, 0.7),                           -- scarf: cozy
    -- jersey stays below the t-shirt (0.6) so the generic catalog pick
    -- remains the universal item; owned jerseys still win in wardrobe mode
    (20, 1, 0.5), (20, 5, 0.4),             -- jersey: energetic, adventurous
    (21, 3, 0.6), (21, 4, 0.5),             -- polo-shirt: confident, relaxed
    (22, 2, 0.8),                           -- cardigan: cozy
    (23, 1, 0.7),                           -- leggings: energetic
    (24, 5, 0.8),                           -- cargo-pants: adventurous
    (25, 3, 0.8),                           -- loafers: confident
    (26, 3, 0.9),                           -- blazer: confident
    (27, 3, 0.7), (27, 2, 0.5),             -- wool-coat: confident, cozy
    (28, 1, 0.5), (28, 4, 0.4),             -- cap: energetic, relaxed
    (29, 2, 0.5),                           -- gloves: cozy
    -- expanded outerwear; kept below the raincoat's rainy-day score
    (30, 3, 0.9), (30, 5, 0.7),             -- leather-jacket: confident, adventurous
    (31, 1, 0.7), (31, 5, 0.5),             -- bomber-jacket: energetic, adventurous
    (32, 2, 0.6), (32, 5, 0.6),             -- parka: cozy, adventurous
    (33, 1, 0.8),                           -- windbreaker: energetic
    (34, 1, 0.5), (34, 4, 0.4),             -- puffer-vest: energetic, relaxed
    (35, 3, 0.7), (35, 2, 0.4),             -- overcoat: confident, cozy
    -- expanded tops; kept below hoodie (cozy 0.9), shirt (confident 0.9)
    -- and t-shirt (energetic 0.6) so the classic picks stay stable
    (36, 2, 0.8), (36, 4, 0.6),             -- sweatshirt: cozy, relaxed
    (37, 1, 0.5), (37, 4, 0.4),             -- tank-top: energetic, relaxed
    (38, 3, 0.8),                           -- blouse: confident
    (39, 2, 0.7), (39, 5, 0.4),             -- flannel-shirt: cozy, adventurous
    -- expanded bottoms
    (40, 3, 0.9),                           -- dress-pants: confident
    (41, 3, 0.6), (41, 1, 0.5),             -- skirt: confident, energetic
    (42, 4, 0.7),                           -- linen-pants: relaxed
    -- expanded footwear
    (43, 3, 0.9),                           -- heels: confident
    (44, 4, 0.6), (44, 3, 0.4),             -- flats: relaxed, confident
    (45, 3, 0.8),                           -- classic-shoes: confident
    -- expanded accessories
    (46, 3, 0.4),                           -- belt: confident
    (47, 3, 0.7),                           -- tie: confident
    (48, 5, 0.6), (48, 1, 0.4),             -- backpack: adventurous, energetic
    (49, 3, 0.5),                           -- handbag: confident
    -- jewelry: temperature-neutral (0.5 fit), so only pieces whose blended
    -- mood weight reaches 0.6 clear the optional-category threshold of 0.8
    (50, 3, 0.6),                           -- necklace: confident
    (51, 3, 0.4), (51, 1, 0.4),             -- earrings: confident, energetic
    (52, 4, 0.4),                           -- bracelet: relaxed
    (53, 3, 0.5),                           -- ring: confident
    (54, 3, 0.7),                           -- watch: confident
    (55, 3, 0.4);                           -- brooch: confident

)sql";

// MSVC caps a single string literal at ~16 KB, so the seed continues in a
// second constant; seedDatabase() executes both.
constexpr const char* kSeedSqlTranslations = R"sql(
INSERT OR IGNORE INTO translations (entity_type, entity_id, lang_code, name) VALUES
    ('category', 1, 'en', 'Outerwear'),     ('category', 1, 'tr', 'Dış giyim'),
    ('category', 2, 'en', 'Top'),           ('category', 2, 'tr', 'Üst giyim'),
    ('category', 3, 'en', 'Bottom'),        ('category', 3, 'tr', 'Alt giyim'),
    ('category', 4, 'en', 'Footwear'),      ('category', 4, 'tr', 'Ayakkabı'),
    ('category', 5, 'en', 'Accessory'),     ('category', 5, 'tr', 'Aksesuar'),
    ('category', 6, 'en', 'Jewelry'),       ('category', 6, 'tr', 'Takı'),

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
    ('item', 20, 'en', 'Jersey'),           ('item', 20, 'tr', 'Forma'),
    ('item', 21, 'en', 'Polo shirt'),       ('item', 21, 'tr', 'Polo tişört'),
    ('item', 22, 'en', 'Cardigan'),         ('item', 22, 'tr', 'Hırka'),
    ('item', 23, 'en', 'Leggings'),         ('item', 23, 'tr', 'Tayt'),
    ('item', 24, 'en', 'Cargo pants'),      ('item', 24, 'tr', 'Kargo pantolon'),
    ('item', 25, 'en', 'Loafers'),          ('item', 25, 'tr', 'Loafer ayakkabı'),
    ('item', 26, 'en', 'Blazer'),           ('item', 26, 'tr', 'Blazer ceket'),
    ('item', 27, 'en', 'Wool coat'),        ('item', 27, 'tr', 'Yün palto'),
    ('item', 28, 'en', 'Cap'),              ('item', 28, 'tr', 'Şapka'),
    ('item', 29, 'en', 'Gloves'),           ('item', 29, 'tr', 'Eldiven'),
    ('item', 30, 'en', 'Leather jacket'),   ('item', 30, 'tr', 'Deri ceket'),
    ('item', 31, 'en', 'Bomber jacket'),    ('item', 31, 'tr', 'Bomber ceket'),
    ('item', 32, 'en', 'Parka'),            ('item', 32, 'tr', 'Parka'),
    ('item', 33, 'en', 'Windbreaker'),      ('item', 33, 'tr', 'Rüzgarlık'),
    ('item', 34, 'en', 'Puffer vest'),      ('item', 34, 'tr', 'Şişme yelek'),
    ('item', 35, 'en', 'Overcoat'),         ('item', 35, 'tr', 'Kaban'),
    ('item', 36, 'en', 'Sweatshirt'),       ('item', 36, 'tr', 'Sweatshirt'),
    ('item', 37, 'en', 'Tank top'),         ('item', 37, 'tr', 'Atlet'),
    ('item', 38, 'en', 'Blouse'),           ('item', 38, 'tr', 'Bluz'),
    ('item', 39, 'en', 'Flannel shirt'),    ('item', 39, 'tr', 'Oduncu gömleği'),
    ('item', 40, 'en', 'Dress pants'),      ('item', 40, 'tr', 'Kumaş pantolon'),
    ('item', 41, 'en', 'Skirt'),            ('item', 41, 'tr', 'Etek'),
    ('item', 42, 'en', 'Linen pants'),      ('item', 42, 'tr', 'Keten pantolon'),
    ('item', 43, 'en', 'Heels'),            ('item', 43, 'tr', 'Topuklu ayakkabı'),
    ('item', 44, 'en', 'Flats'),            ('item', 44, 'tr', 'Babet'),
    ('item', 45, 'en', 'Classic shoes'),    ('item', 45, 'tr', 'Klasik ayakkabı'),
    ('item', 46, 'en', 'Belt'),             ('item', 46, 'tr', 'Kemer'),
    ('item', 47, 'en', 'Tie'),              ('item', 47, 'tr', 'Kravat'),
    ('item', 48, 'en', 'Backpack'),         ('item', 48, 'tr', 'Sırt çantası'),
    ('item', 49, 'en', 'Handbag'),          ('item', 49, 'tr', 'El çantası'),
    ('item', 50, 'en', 'Necklace'),         ('item', 50, 'tr', 'Kolye'),
    ('item', 51, 'en', 'Earrings'),         ('item', 51, 'tr', 'Küpe'),
    ('item', 52, 'en', 'Bracelet'),         ('item', 52, 'tr', 'Bilezik'),
    ('item', 53, 'en', 'Ring'),             ('item', 53, 'tr', 'Yüzük'),
    ('item', 54, 'en', 'Watch'),            ('item', 54, 'tr', 'Kol saati'),
    ('item', 55, 'en', 'Brooch'),           ('item', 55, 'tr', 'Broş'),

    ('mood', 1, 'en', 'Energetic'),         ('mood', 1, 'tr', 'Enerjik'),
    ('mood', 2, 'en', 'Cozy'),              ('mood', 2, 'tr', 'Keyifli'),
    ('mood', 3, 'en', 'Confident'),         ('mood', 3, 'tr', 'Özgüvenli'),
    ('mood', 4, 'en', 'Relaxed'),           ('mood', 4, 'tr', 'Sakin'),
    ('mood', 5, 'en', 'Adventurous'),       ('mood', 5, 'tr', 'Maceracı');

INSERT OR IGNORE INTO attributes (id, slug) VALUES
    (1, 'color'),
    (2, 'material'),
    (3, 'collar'),
    (4, 'fit'),
    (5, 'pattern'),
    (6, 'sleeve'),
    (7, 'metal'),
    (8, 'color-tone');

INSERT OR IGNORE INTO category_attributes (category_id, attribute_id) VALUES
    -- color applies everywhere
    (1, 1), (2, 1), (3, 1), (4, 1), (5, 1),
    -- material for everything except accessories
    (1, 2), (2, 2), (3, 2), (4, 2),
    -- collar only for tops, fit only for bottoms
    (2, 3),
    (3, 4),
    -- pattern for everything except footwear
    (1, 5), (2, 5), (3, 5), (5, 5),
    -- sleeve only for tops
    (2, 6),
    -- jewelry gets color (stones, straps) and metal
    (6, 1), (6, 7),
    -- color tone (vivid, pastel, ...) applies everywhere; it mainly feeds
    -- the classifier and recommender rather than the user's eye
    (1, 8), (2, 8), (3, 8), (4, 8), (5, 8), (6, 8);

INSERT OR IGNORE INTO attribute_values (id, attribute_id, slug) VALUES
    (1,  1, 'black'),   (2,  1, 'white'),  (3,  1, 'gray'),
    (4,  1, 'navy'),    (5,  1, 'blue'),   (6,  1, 'red'),
    (7,  1, 'green'),   (8,  1, 'beige'),  (9,  1, 'brown'),
    (10, 1, 'yellow'),  (11, 1, 'pink'),   (12, 1, 'purple'),
    (13, 2, 'cotton'),  (14, 2, 'linen'),  (15, 2, 'wool'),
    (16, 2, 'denim'),   (17, 2, 'leather'),(18, 2, 'synthetic'),
    (19, 3, 'crew-neck'), (20, 3, 'v-neck'), (21, 3, 'polo-collar'), (22, 3, 'turtleneck'),
    (23, 4, 'slim-fit'),  (24, 4, 'regular-fit'), (25, 4, 'wide-leg'),
    (26, 5, 'solid'),     (27, 5, 'striped'),    (28, 5, 'plaid'),
    (29, 5, 'floral'),    (30, 5, 'polka-dot'),  (31, 5, 'graphic'),
    (32, 5, 'camouflage'),
    (33, 6, 'long-sleeve'), (34, 6, 'short-sleeve'), (35, 6, 'sleeveless'),
    (36, 2, 'silk'),      (37, 2, 'corduroy'),
    (38, 7, 'gold'),      (39, 7, 'silver'),     (40, 7, 'rose-gold'),
    (41, 7, 'steel'),
    (42, 8, 'vivid'),     (43, 8, 'pastel'),     (44, 8, 'muted'),
    (45, 8, 'dark'),      (46, 8, 'light'),      (47, 8, 'neutral');

INSERT OR IGNORE INTO translations (entity_type, entity_id, lang_code, name) VALUES
    ('attribute', 1, 'en', 'Color'),        ('attribute', 1, 'tr', 'Renk'),
    ('attribute', 2, 'en', 'Material'),     ('attribute', 2, 'tr', 'Kumaş'),
    ('attribute', 3, 'en', 'Collar'),       ('attribute', 3, 'tr', 'Yaka'),
    ('attribute', 4, 'en', 'Fit'),          ('attribute', 4, 'tr', 'Kesim'),
    ('attribute', 5, 'en', 'Pattern'),      ('attribute', 5, 'tr', 'Desen'),
    ('attribute', 6, 'en', 'Sleeve'),       ('attribute', 6, 'tr', 'Kol'),
    ('attribute', 7, 'en', 'Metal'),        ('attribute', 7, 'tr', 'Metal'),
    ('attribute', 8, 'en', 'Color tone'),   ('attribute', 8, 'tr', 'Renk tonu'),

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
    ('attribute_value', 25, 'en', 'Wide leg'),   ('attribute_value', 25, 'tr', 'Bol paça'),
    ('attribute_value', 26, 'en', 'Solid'),      ('attribute_value', 26, 'tr', 'Düz'),
    ('attribute_value', 27, 'en', 'Striped'),    ('attribute_value', 27, 'tr', 'Çizgili'),
    ('attribute_value', 28, 'en', 'Plaid'),      ('attribute_value', 28, 'tr', 'Ekose'),
    ('attribute_value', 29, 'en', 'Floral'),     ('attribute_value', 29, 'tr', 'Çiçekli'),
    ('attribute_value', 30, 'en', 'Polka dot'),  ('attribute_value', 30, 'tr', 'Puantiyeli'),
    ('attribute_value', 31, 'en', 'Graphic'),    ('attribute_value', 31, 'tr', 'Baskılı'),
    ('attribute_value', 32, 'en', 'Camouflage'), ('attribute_value', 32, 'tr', 'Kamuflaj'),
    ('attribute_value', 33, 'en', 'Long sleeve'),  ('attribute_value', 33, 'tr', 'Uzun kollu'),
    ('attribute_value', 34, 'en', 'Short sleeve'), ('attribute_value', 34, 'tr', 'Kısa kollu'),
    ('attribute_value', 35, 'en', 'Sleeveless'),   ('attribute_value', 35, 'tr', 'Kolsuz'),
    ('attribute_value', 36, 'en', 'Silk'),       ('attribute_value', 36, 'tr', 'İpek'),
    ('attribute_value', 37, 'en', 'Corduroy'),   ('attribute_value', 37, 'tr', 'Fitilli kadife'),
    ('attribute_value', 38, 'en', 'Gold'),       ('attribute_value', 38, 'tr', 'Altın'),
    ('attribute_value', 39, 'en', 'Silver'),     ('attribute_value', 39, 'tr', 'Gümüş'),
    ('attribute_value', 40, 'en', 'Rose gold'),  ('attribute_value', 40, 'tr', 'Roze altın'),
    ('attribute_value', 41, 'en', 'Steel'),      ('attribute_value', 41, 'tr', 'Çelik'),
    ('attribute_value', 42, 'en', 'Vivid'),      ('attribute_value', 42, 'tr', 'Canlı'),
    ('attribute_value', 43, 'en', 'Pastel'),     ('attribute_value', 43, 'tr', 'Pastel'),
    ('attribute_value', 44, 'en', 'Muted'),      ('attribute_value', 44, 'tr', 'Soluk'),
    ('attribute_value', 45, 'en', 'Dark'),       ('attribute_value', 45, 'tr', 'Koyu'),
    ('attribute_value', 46, 'en', 'Light'),      ('attribute_value', 46, 'tr', 'Açık'),
    ('attribute_value', 47, 'en', 'Neutral'),    ('attribute_value', 47, 'tr', 'Nötr');
)sql";

}  // namespace

void seedDatabase(Database& db) {
    db.execute(kSeedSql);
    db.execute(kSeedSqlTranslations);
}

}  // namespace negiysem
