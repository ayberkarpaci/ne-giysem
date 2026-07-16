const STRINGS = {
    en: {
        mood_title: "How do you feel today?",
        feel_placeholder: "e.g. tired after a long week, but excited for tonight",
        mood_quick: "...or pick one quickly:",
        mood_line: (moods) =>
            "How you seem to feel: " +
            moods.map((m) => `${m.name} ${Math.round(m.weight * 100)}%`).join(" · "),
        recommend: "What should I wear?",
        loading: "Checking the weather...",
        weather: (w) => {
            const t = Math.round(w.temperature_c * 10) / 10;
            const rainy = w.is_raining ? "rainy" : "dry";
            const city = w.city ? `${w.city}: ` : "";
            if (w.basis === "manual") return `Weather you entered: ${t} °C, ${rainy}`;
            if (w.basis === "window")
                return `${city}${hh(w.start_hour)}–${hh(w.end_hour)} average ` +
                       `${t} °C, ${rainy} (forecast)`;
            if (w.basis === "daily") return `${city}${t} °C, ${rainy} (day average)`;
            return `${city}${t} °C, ${rainy} (right now)`;
        },
        error: "Could not get a recommendation. Is the network up?",
        save_error: "Could not save the item.",
        footer: "Weather & city search by open-meteo.com · location by ip-api.com",
        weather_title: "Weather & location",
        location_label: "Location",
        change: "change",
        auto_locate: "Use my IP location",
        city_placeholder: "Type a city, e.g. Aydın",
        hours_label: "When will you be out?",
        manual_weather: "I'll enter the weather myself",
        temp_label: "Temperature (°C)",
        rain_label: "Rainy",
        locating: "Locating...",
        location_unknown: "could not detect",
        gender_label: "Catalog suggestions for",
        gender_any: "everyone",
        gender_male: "men",
        gender_female: "women",
        use_wardrobe: "Recommend from my wardrobe",
        catalog_fallback: "Your wardrobe is empty, so this comes from the general catalog.",
        ask_title: "Where are you heading?",
        ask_placeholder: "e.g. going to work tomorrow, smart but comfy",
        ask: "Suggest an outfit",
        asking: "Thinking...",
        stage_eyebrow: "Today's outfit",
        why_title: "Why it works",
        pieces_title: "Chosen pieces",
        piece_count: (n) => `${n} PIECE${n === 1 ? "" : "S"}`,
        recent_title: "Recent looks",
        recent_hint: "Tap one to bring it back",
        edit_context: "Adjust details ▾",
        fallback_title: "Today's pick",
        confidence_label: "Confidence",
        "check_weather-fit": "Fits the weather",
        "check_rain-ready": "Ready for rain",
        "check_formality-consistent": "Dress code consistent",
        "check_colors-harmonious": "Colors in harmony",
        "check_patterns-calm": "Patterns balanced",
        "check_loved-pair": "A duo you loved before",
        "check_style-match": "Matches your style profile",
        analytics_tab: "Analysis",
        an_total: "pieces",
        an_dirty: "in the laundry",
        an_never: "never worn",
        an_cutout: "with clean cutouts",
        an_categories: "By category",
        an_colors: "Color palette",
        an_most_worn: "Most worn",
        an_gaps: "Wardrobe gaps",
        an_no_gaps: "No gaps — every core category covers cold, mild and hot days. 👌",
        an_no_wears: "No wears logged yet — tap \"I wore this\" under a recommendation.",
        gap_line: (category, band) =>
            `No ${category.toLowerCase()} for ${band === "cold" ? "cold" : band === "hot" ? "hot" : "mild"} days`,
        wears_unit: (n) => `${n}×`,
        ask_no_key: "The Gemini API key is not set up yet (see .env).",
        wardrobe_title: "My wardrobe",
        wardrobe_empty: "No items yet — add your first piece below!",
        add_item: "Add a single item",
        type_label: "Type",
        label_label: "Name (optional)",
        photo_label: "Photo (optional)",
        save: "Save",
        saving: "Saving...",
        delete: "Delete",
        choose: "—",
        classifying: "Reading the photo...",
        classified: "✨ Filled in from the photo — check the fields and save.",
        classify_error: "Could not recognize the photo; pick the fields yourself.",
        rate_title: "Rate this suggestion:",
        comment_placeholder: "Add a comment (optional)",
        send_feedback: "Send",
        feedback_thanks: "Thanks! I'll learn from this.",
        revised_note: "Not your style — here's another idea:",
        feedback_error: "Could not send the feedback.",
        tab_home: "Outfit",
        tab_wardrobe: "My wardrobe",
        bulk_title: "Add many at once",
        bulk_hint: "Pick several garment photos — each one is recognized and saved automatically.",
        bulk_pick: "📸 Choose photos",
        bulk_reading: "recognizing...",
        bulk_saved: "added",
        bulk_failed: "not recognized — add it manually below",
        bulk_rate_limited: "API limit reached — press Retry in a moment",
        bulk_retry: "Retry",
        bulk_summary: (ok, total) => `${ok}/${total} added to your wardrobe.`,
        filter_all: "All",
        extract_all: (n) => `✨ Clean up backgrounds (${n})`,
        extracting: (done, total) => `Cleaning up ${done}/${total}… this takes a moment per item.`,
        extract_done: "Backgrounds cleaned up ✓",
        extract_failed: (n) => `${n} item(s) could not be cleaned — try again later.`,
        extract_rate_limited: "API limit reached — try again in a minute.",
        extract_no_tool: "The local background remover (rembg) is not set up — see the README.",
        look_title: "The look",
        add_button: "📸 Add clothes",
        items_count: (n) => `${n} item${n === 1 ? "" : "s"}`,
        outfits_count: (n) => `${n} outfit${n === 1 ? "" : "s"}`,
        outfits_tab: "Outfits",
        look_word: "Look",
        panel_name: "Name",
        panel_category: "Category",
        panel_colors: "Colors",
        panel_primary: "Selected",
        panel_palette: "Image suggestions",
        panel_details: "Details",
        panel_no_details: "No details recorded.",
        no_outfits: "No outfits yet — ask for a recommendation on the Outfit tab and it will be saved here.",
        name_saved: "Saved ✓",
        mood_tag: (name) => name,
        outfit_desc_fallback: "Put together for your weather and mood.",
        stage_hint: "Tell me your plan or tap \"What should I wear?\" — your outfit appears here.",
        create_outfit: "➕ Create an outfit",
        builder_name: "Outfit name (optional)",
        builder_pick: "Tap pieces to add them to the look:",
        builder_save: "Save outfit",
        builder_need: "Pick at least two pieces.",
        delete_outfit: "🗑 Delete this look",
        worn_button: "👕 I wore this",
        worn_thanks: (n) => `Noted — ${n} piece${n === 1 ? "" : "s"} logged.`,
        worn_error: "Could not log the wear.",
        mark_dirty: "🧺 To the laundry",
        mark_clean: "✨ Washed & clean",
        dirty_note: "In the laundry — left out of outfit suggestions.",
        last_worn: (date) => `Last worn: ${date}`,
        never_worn: "Not worn yet.",
        why_title: "What was off? (optional)",
        "tag_too-hot": "🥵 Too warm",
        "tag_too-cold": "🥶 Too cold",
        "tag_colors-clash": "🎨 Colors clash",
        "tag_too-formal": "👔 Too formal",
        "tag_too-sporty": "👟 Too sporty",
        "tag_uncomfortable": "😣 Uncomfortable",
    },
    tr: {
        mood_title: "Bugün nasıl hissediyorsun?",
        feel_placeholder: "örn. yorucu bir hafta oldu ama akşam için heyecanlıyım",
        mood_quick: "...ya da hızlıca seç:",
        mood_line: (moods) =>
            "Anladığım ruh hâli: " +
            moods.map((m) => `%${Math.round(m.weight * 100)} ${m.name}`).join(" · "),
        recommend: "Ne giysem?",
        loading: "Hava durumuna bakılıyor...",
        weather: (w) => {
            const t = Math.round(w.temperature_c * 10) / 10;
            const rainy = w.is_raining ? "yağmurlu" : "kuru";
            const city = w.city ? `${w.city}: ` : "";
            if (w.basis === "manual") return `Senin girdiğin hava: ${t} °C, ${rainy}`;
            if (w.basis === "window")
                return `${city}${hh(w.start_hour)}–${hh(w.end_hour)} arası ort. ` +
                       `${t} °C, ${rainy} (tahmin)`;
            if (w.basis === "daily") return `${city}${t} °C, ${rainy} (gün ortalaması)`;
            return `${city}${t} °C, ${rainy} (şu an)`;
        },
        error: "Öneri alınamadı. Ağ bağlantısını kontrol et.",
        save_error: "Kıyafet kaydedilemedi.",
        footer: "Hava durumu ve şehir arama: open-meteo.com · konum: ip-api.com",
        weather_title: "Hava & konum",
        location_label: "Konum",
        change: "değiştir",
        auto_locate: "IP konumumu kullan",
        city_placeholder: "Şehir yaz, örn. Aydın",
        hours_label: "Ne zaman dışarıda olacaksın?",
        manual_weather: "Hava durumunu kendim gireceğim",
        temp_label: "Sıcaklık (°C)",
        rain_label: "Yağışlı",
        locating: "Konum bulunuyor...",
        location_unknown: "bulunamadı",
        gender_label: "Katalog önerileri kime göre",
        gender_any: "herkes",
        gender_male: "erkek",
        gender_female: "kadın",
        use_wardrobe: "Gardırobumdan öner",
        catalog_fallback: "Gardırobun boş olduğu için bu öneri genel katalogdan geldi.",
        ask_title: "Nereye gidiyorsun?",
        ask_placeholder: "örn. yarın işe gideceğim, şık ama rahat olsun",
        ask: "Kombin öner",
        asking: "Düşünüyorum...",
        stage_eyebrow: "Bugünün kombini",
        why_title: "Neden çalışıyor?",
        pieces_title: "Seçilen parçalar",
        piece_count: (n) => `${n} PARÇA`,
        recent_title: "Son kombinler",
        recent_hint: "Geri getirmek için dokun",
        edit_context: "Ayrıntıları düzenle ▾",
        fallback_title: "Bugünün önerisi",
        confidence_label: "Güven",
        "check_weather-fit": "Havaya uygun",
        "check_rain-ready": "Yağmura hazır",
        "check_formality-consistent": "Resmiyet tutarlı",
        "check_colors-harmonious": "Renkler uyumlu",
        "check_patterns-calm": "Desenler dengeli",
        "check_loved-pair": "Daha önce sevdiğin bir ikili",
        "check_style-match": "Stil profiline uygun",
        analytics_tab: "Analiz",
        an_total: "parça",
        an_dirty: "kirli sepetinde",
        an_never: "hiç giyilmemiş",
        an_cutout: "temiz kesimli",
        an_categories: "Kategoriye göre",
        an_colors: "Renk paleti",
        an_most_worn: "En çok giyilenler",
        an_gaps: "Gardırop boşlukları",
        an_no_gaps: "Boşluk yok — her temel kategori soğuk, ılık ve sıcak günleri kapsıyor. 👌",
        an_no_wears: "Henüz giyme kaydı yok — öneri altındaki \"Bunu giydim\"e dokun.",
        gap_line: (category, band) =>
            `${band === "cold" ? "Soğuk" : band === "hot" ? "Sıcak" : "Ilık"} günler için ${category.toLowerCase()} eksik`,
        wears_unit: (n) => `${n} kez`,
        ask_no_key: "Gemini API anahtarı henüz ayarlanmamış (.env dosyasına bak).",
        wardrobe_title: "Gardırobum",
        wardrobe_empty: "Henüz kıyafet yok — aşağıdan ilk parçanı ekle!",
        add_item: "Tek kıyafet ekle",
        type_label: "Tür",
        label_label: "İsim (isteğe bağlı)",
        photo_label: "Fotoğraf (isteğe bağlı)",
        save: "Kaydet",
        saving: "Kaydediliyor...",
        delete: "Sil",
        choose: "—",
        classifying: "Fotoğraf inceleniyor...",
        classified: "✨ Fotoğraftan dolduruldu — alanları kontrol edip kaydet.",
        classify_error: "Fotoğraf tanınamadı; alanları kendin seçebilirsin.",
        rate_title: "Bu öneriyi puanla:",
        comment_placeholder: "İstersen yorum ekle (isteğe bağlı)",
        send_feedback: "Gönder",
        feedback_thanks: "Teşekkürler! Bundan ders çıkaracağım.",
        revised_note: "Beğenmedin — işte başka bir fikir:",
        feedback_error: "Geri bildirim gönderilemedi.",
        tab_home: "Kombin",
        tab_wardrobe: "Gardırobum",
        bulk_title: "Toplu kıyafet ekle",
        bulk_hint: "Birden fazla kıyafet fotoğrafı seç — her biri otomatik tanınıp kaydedilir.",
        bulk_pick: "📸 Fotoğrafları seç",
        bulk_reading: "tanınıyor...",
        bulk_saved: "eklendi",
        bulk_failed: "tanınamadı — aşağıdan elle ekleyebilirsin",
        bulk_rate_limited: "API sınırına takıldı — az sonra Tekrar dene",
        bulk_retry: "Tekrar dene",
        bulk_summary: (ok, total) => `${ok}/${total} gardırobuna eklendi.`,
        filter_all: "Hepsi",
        extract_all: (n) => `✨ Arka planları temizle (${n})`,
        extracting: (done, total) => `Temizleniyor ${done}/${total}… her parça biraz sürüyor.`,
        extract_done: "Arka planlar temizlendi ✓",
        extract_failed: (n) => `${n} parça temizlenemedi — sonra tekrar dene.`,
        extract_rate_limited: "API sınırına takıldı — bir dakika sonra tekrar dene.",
        extract_no_tool: "Yerel arka plan temizleyici (rembg) kurulu değil — README'ye bak.",
        look_title: "Kombin",
        add_button: "📸 Kıyafet ekle",
        items_count: (n) => `${n} parça`,
        outfits_count: (n) => `${n} kombin`,
        outfits_tab: "Kombinler",
        look_word: "Kombin",
        panel_name: "İsim",
        panel_category: "Kategori",
        panel_colors: "Renkler",
        panel_primary: "Seçilen",
        panel_palette: "Fotoğraftan öneriler",
        panel_details: "Detaylar",
        panel_no_details: "Kayıtlı detay yok.",
        no_outfits: "Henüz kombin yok — Kombin sekmesinden öneri iste, buraya kaydedilsin.",
        name_saved: "Kaydedildi ✓",
        mood_tag: (name) => name,
        outfit_desc_fallback: "Havana ve ruh hâline göre bir araya getirildi.",
        stage_hint: "Planını anlat ya da \"Ne giysem?\"e dokun — kombinin burada belirecek.",
        create_outfit: "➕ Kombin oluştur",
        builder_name: "Kombin adı (isteğe bağlı)",
        builder_pick: "Kombine eklemek için parçalara dokun:",
        builder_save: "Kombini kaydet",
        builder_need: "En az iki parça seç.",
        delete_outfit: "🗑 Bu kombini sil",
        worn_button: "👕 Bunu giydim",
        worn_thanks: (n) => `Not edildi — ${n} parça kaydedildi.`,
        worn_error: "Giyme kaydedilemedi.",
        mark_dirty: "🧺 Kirliye at",
        mark_clean: "✨ Yıkandı, temiz",
        dirty_note: "Kirli sepetinde — kombin önerilerine girmiyor.",
        last_worn: (date) => `Son giyilme: ${date}`,
        never_worn: "Henüz giyilmedi.",
        why_title: "Sorun neydi? (isteğe bağlı)",
        "tag_too-hot": "🥵 Fazla terletir",
        "tag_too-cold": "🥶 Üşütür",
        "tag_colors-clash": "🎨 Renkler uyumsuz",
        "tag_too-formal": "👔 Fazla resmi",
        "tag_too-sporty": "👟 Fazla spor",
        "tag_uncomfortable": "😣 Rahat değil",
    },
};

// One-tap reasons the user can attach to a rating; slugs match the server's
// controlled vocabulary (FeedbackRepository::allowedTags).
const FEEDBACK_TAGS = ["too-hot", "too-cold", "colors-clash", "too-formal",
                       "too-sporty", "uncomfortable"];

const CATEGORY_EMOJI = {
    outerwear: "🧥",
    top: "👕",
    bottom: "👖",
    "one-piece": "👗",
    footwear: "👟",
    accessory: "🧣",
    jewelry: "💍",
};

let lang = localStorage.getItem("lang") || "en";
let mood = null;  // no quick-pick selected until the user chooses one
let wardrobeCount = 0;
// User-corrected location {city, latitude, longitude}; null = detect by IP.
let savedLocation = JSON.parse(localStorage.getItem("location") || "null");
let detectedCity = "";

const $ = (id) => document.getElementById(id);
const t = (key) => STRINGS[lang][key];
const hh = (h) => String(h).padStart(2, "0") + ":00";

function showError(message, target = "error") {
    $(target).textContent = message;
    $(target).classList.remove("hidden");
}

function applyStaticStrings() {
    document.documentElement.lang = lang;
    document.querySelectorAll("[data-i18n]").forEach((el) => {
        const value = STRINGS[lang][el.dataset.i18n];
        if (typeof value === "string") el.textContent = value;
    });
    document.querySelectorAll(".lang-switch button").forEach((btn) => {
        btn.classList.toggle("active", btn.dataset.lang === lang);
    });
    $("ask-input").placeholder = t("ask_placeholder");
    $("feel-input").placeholder = t("feel_placeholder");
    $("city-input").placeholder = t("city_placeholder");
    $("add-toggle-btn").textContent = "+";
    $("add-toggle-btn").title = t("add_button");
    renderLocationName();
}

function renderLocationName() {
    $("location-name").textContent =
        savedLocation ? savedLocation.city
                      : detectedCity || t("location_unknown");
    const city = savedLocation ? savedLocation.city : detectedCity;
    if (city && $("top-weather").classList.contains("hidden")) {
        $("top-weather").textContent = `📍 ${city}`;
        $("top-weather").classList.remove("hidden");
    }
    renderContextChips();
}

async function loadLocation() {
    if (savedLocation) {
        renderLocationName();
        return;
    }
    $("location-name").textContent = t("locating");
    try {
        const data = await fetchJson("/api/location");
        detectedCity = data.city;
    } catch (err) {
        console.error(err);
        detectedCity = "";
    }
    renderLocationName();
}

let citySearchTimer = null;
async function searchCity() {
    const name = $("city-input").value.trim();
    const results = $("city-results");
    if (name.length < 2) {
        results.innerHTML = "";
        return;
    }
    try {
        const data = await fetchJson(
            `/api/geocode?name=${encodeURIComponent(name)}&lang=${lang}`);
        results.innerHTML = "";
        for (const candidate of data.results) {
            const btn = document.createElement("button");
            btn.type = "button";
            btn.textContent = candidate.city;
            btn.addEventListener("click", () => {
                savedLocation = candidate;
                localStorage.setItem("location", JSON.stringify(candidate));
                $("location-editor").classList.add("hidden");
                $("city-input").value = "";
                results.innerHTML = "";
                renderLocationName();
            });
            results.appendChild(btn);
        }
    } catch (err) {
        console.error(err);
    }
}

function initHourSelects() {
    const saved = JSON.parse(localStorage.getItem("hours") || '{"start":9,"end":18}');
    for (const [id, value] of [["hour-start", saved.start], ["hour-end", saved.end]]) {
        const select = $(id);
        for (let h = 0; h < 24; ++h) {
            const option = document.createElement("option");
            option.value = h;
            option.textContent = hh(h);
            select.appendChild(option);
        }
        select.value = value;
        select.addEventListener("change", () => {
            localStorage.setItem("hours", JSON.stringify({
                start: Number($("hour-start").value),
                end: Number($("hour-end").value),
            }));
        });
    }
}

// The weather/location/profile choices every recommendation request carries.
function weatherOptions() {
    const options = {
        start_hour: Number($("hour-start").value),
        end_hour: Number($("hour-end").value),
    };
    if ($("gender-select").value) options.gender = $("gender-select").value;
    if (savedLocation) {
        options.lat = savedLocation.latitude;
        options.lon = savedLocation.longitude;
        options.city = savedLocation.city;
    }
    if ($("manual-weather").checked) {
        const temp = parseFloat($("manual-temp").value);
        if (!Number.isNaN(temp)) {
            options.temp = temp;
            options.rain = $("manual-rain").checked;
        }
    }
    return options;
}

function weatherQueryString() {
    const o = weatherOptions();
    let query = `&start_hour=${o.start_hour}&end_hour=${o.end_hour}`;
    if (o.lat !== undefined) {
        query += `&lat=${o.lat}&lon=${o.lon}&city=${encodeURIComponent(o.city)}`;
    }
    if (o.temp !== undefined) query += `&temp=${o.temp}&rain=${o.rain ? 1 : 0}`;
    if (o.gender) query += `&gender=${o.gender}`;
    return query;
}

// Cutouts composed like a flat-lay "look": tops up, bottoms middle, shoes
// and extras below. Items without a cutout stay in the list underneath.
function buildLookBoard(items) {
    const withCutout = items.filter((item) => item.cutout_url);
    if (withCutout.length < 2) return null;
    const rows = [["outerwear", "top"], ["one-piece", "bottom"],
                  ["footwear", "accessory", "jewelry"]];
    const board = document.createElement("div");
    board.className = "look-board";
    for (const categories of rows) {
        const row = document.createElement("div");
        row.className = "look-row";
        for (const category of categories) {
            for (const item of withCutout) {
                if (item.category_slug !== category) continue;
                const img = document.createElement("img");
                img.src = item.cutout_url;
                img.alt = item.item_name;
                img.className = `look-piece look-${category}`;
                row.appendChild(img);
            }
        }
        if (row.children.length > 0) board.appendChild(row);
    }
    return board.children.length > 1 ? board : null;
}

function renderOutfitList(listEl, items) {
    listEl.innerHTML = "";
    for (const item of items) {
        const li = document.createElement("li");
        const visual = document.createElement("span");
        if (item.cutout_url || item.photo_url) {
            const img = document.createElement("img");
            img.src = item.cutout_url || item.photo_url;
            img.alt = item.item_name;
            img.className = item.cutout_url ? "thumb thumb-cutout" : "thumb";
            visual.appendChild(img);
        } else {
            visual.className = "emoji";
            visual.textContent = CATEGORY_EMOJI[item.category_slug] || "👔";
        }
        const text = document.createElement("span");
        const category = document.createElement("span");
        category.className = "category";
        category.textContent = item.category_name;
        const name = document.createElement("span");
        name.textContent = item.item_name;
        text.append(category, name);
        li.append(visual, text);
        listEl.appendChild(li);
    }
}

async function fetchJson(url, options) {
    const res = await fetch(url, options);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return res.json();
}

// ---- the stage: one render path for every outfit source ----

function renderStageBoard(items) {
    const host = $("stage-board");
    host.innerHTML = "";
    const board = buildLookBoard(items);
    if (board) host.appendChild(board);
}

// Fills the whole home screen from one response: title and weather in the
// stage header, the big board in the middle, reason + pieces + feedback in
// the sidebar.
function renderStage(data) {
    $("stage-empty").classList.add("hidden");
    $("outfit-title").textContent = data.title || t("fallback_title");
    $("weather-line").textContent = data.weather ? t("weather")(data.weather) : "";
    renderStageBoard(data.outfit);
    renderOutfitList($("outfit-list"), data.outfit);
    $("item-count").textContent = t("piece_count")(data.outfit.length);
    $("explanation").textContent = data.explanation || "";
    renderInsights(data.insights);
    $("why-section").classList.toggle(
        "hidden", !data.explanation && !data.insights);
    renderFeedback("feedback-box", data.recommendation_id, "outfit-list");
    if (data.weather) {
        const w = data.weather;
        $("top-weather").textContent =
            `${w.city ? w.city + " · " : ""}${Math.round(w.temperature_c)}°` +
            `${w.is_raining ? " 🌧" : " ☀"}`;
        $("top-weather").classList.remove("hidden");
    }
    savedOutfits = null;  // the look was saved; refresh the tray + OUTFITS tab
    renderRecentLooks();
}

// The film strip under the stage: the last few saved looks, tappable.
async function renderRecentLooks() {
    if (savedOutfits === null) {
        try {
            savedOutfits = (await fetchJson(`/api/outfits?lang=${lang}`)).outfits;
        } catch (err) {
            console.error(err);
            return;
        }
    }
    const tray = $("recent-tray");
    const host = $("recent-looks");
    host.innerHTML = "";
    const recent = savedOutfits
        .filter((o) => o.items.some((i) => i.cutout_url || i.photo_url))
        .slice(0, 4);
    tray.classList.toggle("hidden", recent.length === 0);
    for (const outfit of recent) {
        const card = document.createElement("button");
        card.type = "button";
        card.className = "look-card";
        const thumb = document.createElement("span");
        thumb.className = "look-thumb";
        for (const item of outfit.items.filter((i) => i.cutout_url || i.photo_url)
                                       .slice(0, 2)) {
            const img = document.createElement("img");
            img.src = item.cutout_url || item.photo_url;
            img.alt = item.item_name;
            img.loading = "lazy";
            thumb.appendChild(img);
        }
        const copy = document.createElement("span");
        copy.className = "look-copy";
        copy.innerHTML = "";
        const line1 = document.createElement("strong");
        line1.textContent = outfit.title || `${t("look_word")} ${outfit.id}`;
        const line2 = document.createElement("span");
        line2.textContent = outfit.items.map((i) => i.item_name).slice(0, 2).join(" · ");
        copy.append(line1, line2);
        card.append(thumb, copy);
        card.addEventListener("click", () => {
            renderStage({
                title: outfit.title,
                outfit: outfit.items.map((i) => ({ ...i, score: 0 })),
                explanation: outfit.explanation,
                recommendation_id: outfit.id,
            });
        });
        host.appendChild(card);
    }
}

// The structured explanation: a confidence meter plus the checklist of
// reasons the stylist engine verified (or flagged).
function renderInsights(insights) {
    const box = $("insights");
    if (!insights || !insights.checks || insights.checks.length === 0) {
        box.classList.add("hidden");
        return;
    }
    box.classList.remove("hidden");
    $("confidence-value").textContent = `%${insights.confidence}`;
    $("confidence-fill").style.width = `${insights.confidence}%`;
    const list = $("check-list");
    list.innerHTML = "";
    for (const check of insights.checks) {
        const li = document.createElement("li");
        li.className = check.ok ? "ok" : "off";
        const icon = document.createElement("span");
        icon.className = "check-icon";
        icon.textContent = check.ok ? "✓" : "✕";
        const label = document.createElement("span");
        label.textContent = t(`check_${check.slug}`) || check.slug;
        li.append(icon, label);
        list.appendChild(li);
    }
}

// The compact context chips under the prompt; tapping them (or the link)
// opens the detailed editor.
function renderContextChips() {
    const host = $("context-chips");
    host.innerHTML = "";
    const add = (text) => {
        const chip = document.createElement("button");
        chip.type = "button";
        chip.className = "chip";
        chip.textContent = text;
        chip.addEventListener("click", toggleContextEditor);
        host.appendChild(chip);
    };
    add(`◷ ${hh(Number($("hour-start").value || 9)).slice(0, 5)}–` +
        `${hh(Number($("hour-end").value || 18)).slice(0, 5)}`);
    if ($("manual-weather").checked) {
        add(`🌡 ${$("manual-temp").value}°${$("manual-rain").checked ? " 🌧" : ""}`);
    } else {
        add(`📍 ${savedLocation ? savedLocation.city : detectedCity || "…"}`);
    }
    if (mood) {
        const chipEl = document.querySelector(`#mood-chips button[data-slug="${mood}"]`);
        add(`✦ ${chipEl ? chipEl.textContent : mood}`);
    } else if ($("feel-input").value.trim()) {
        add("✦ …");
    }
    if ($("gender-select").value) {
        add($("gender-select").selectedOptions[0].textContent);
    }
}

function toggleContextEditor() {
    $("context-editor").classList.toggle("hidden");
}

// Phone photos are often 5-10 MB, which makes classification slow and
// flaky. Downscale to a JPEG that fits comfortably in one API call.
async function shrinkPhoto(file, maxSide = 1280) {
    try {
        const bitmap = await createImageBitmap(file);
        const scale = Math.min(1, maxSide / Math.max(bitmap.width, bitmap.height));
        if (scale === 1 && file.size < 1.5 * 1024 * 1024) return file;
        const canvas = document.createElement("canvas");
        canvas.width = Math.round(bitmap.width * scale);
        canvas.height = Math.round(bitmap.height * scale);
        canvas.getContext("2d").drawImage(bitmap, 0, 0, canvas.width, canvas.height);
        const blob = await new Promise((r) => canvas.toBlob(r, "image/jpeg", 0.85));
        if (!blob) return file;
        return new File([blob], file.name.replace(/\.\w+$/, "") + ".jpg",
                        { type: "image/jpeg" });
    } catch (err) {
        console.error("shrink failed, sending original", err);
        return file;
    }
}

// Star rating + optional comment under an outfit. A poor rating may get a
// revised outfit back, which replaces the list and gets its own widget.
function renderFeedback(boxId, recommendationId, listId) {
    const box = $(boxId);
    box.innerHTML = "";
    if (recommendationId) appendFeedbackWidget(box, recommendationId, listId);
}

function appendFeedbackWidget(box, recommendationId, listId) {
    const note = (key) => {
        const p = document.createElement("p");
        p.className = "note";
        p.textContent = t(key);
        return p;
    };
    const widget = document.createElement("div");

    // "I wore this" logs a wear for every wardrobe piece in the outfit,
    // which feeds variety and the laundry state.
    const worn = document.createElement("button");
    worn.type = "button";
    worn.className = "ghost-btn worn-btn";
    worn.textContent = t("worn_button");
    worn.addEventListener("click", async () => {
        worn.disabled = true;
        try {
            const data = await fetchJson(`/api/recommendations/${recommendationId}/worn`,
                                         { method: "POST" });
            const thanks = document.createElement("p");
            thanks.className = "note";
            thanks.textContent = t("worn_thanks")(data.worn);
            worn.replaceWith(thanks);
            await loadWardrobe();  // dirty badges may have appeared
        } catch (err) {
            console.error(err);
            worn.disabled = false;
            showError(t("worn_error"));
        }
    });
    widget.appendChild(worn);

    widget.appendChild(note("rate_title"));

    let rating = 0;
    const stars = document.createElement("div");
    stars.className = "stars";
    const starButtons = [];
    for (let i = 1; i <= 5; ++i) {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.textContent = "☆";
        btn.addEventListener("click", () => {
            rating = i;
            starButtons.forEach((s, idx) => (s.textContent = idx < i ? "★" : "☆"));
            why.classList.remove("hidden");
            detail.classList.remove("hidden");
        });
        starButtons.push(btn);
        stars.appendChild(btn);
    }
    widget.appendChild(stars);

    // One-tap reasons, revealed together with the comment box. They both
    // explain the rating and steer the retry after a poor one.
    const why = document.createElement("div");
    why.className = "hidden";
    why.appendChild(note("why_title"));
    const tagChips = document.createElement("div");
    tagChips.className = "chips feedback-tags";
    const selectedTags = new Set();
    for (const slug of FEEDBACK_TAGS) {
        const chip = document.createElement("button");
        chip.type = "button";
        chip.textContent = t(`tag_${slug}`);
        chip.addEventListener("click", () => {
            if (selectedTags.has(slug)) {
                selectedTags.delete(slug);
            } else {
                selectedTags.add(slug);
            }
            chip.classList.toggle("selected", selectedTags.has(slug));
        });
        tagChips.appendChild(chip);
    }
    why.appendChild(tagChips);
    widget.appendChild(why);

    const detail = document.createElement("div");
    detail.className = "feedback-detail hidden";
    const comment = document.createElement("textarea");
    comment.rows = 2;
    comment.maxLength = 300;
    comment.placeholder = t("comment_placeholder");
    const send = document.createElement("button");
    send.type = "button";
    send.className = "send-feedback";
    send.textContent = t("send_feedback");
    send.addEventListener("click", async () => {
        if (!rating) return;
        send.disabled = true;
        try {
            const data = await fetchJson("/api/feedback", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    recommendation_id: recommendationId,
                    rating,
                    comment: comment.value.trim(),
                    tags: [...selectedTags],
                    lang,
                    gender: $("gender-select").value || undefined,
                }),
            });
            box.innerHTML = "";
            box.appendChild(note("feedback_thanks"));
            if (data.outfit) {
                box.appendChild(note("revised_note"));
                renderOutfitList($(listId), data.outfit);
                if (listId === "outfit-list") renderStageBoard(data.outfit);
                if (data.recommendation_id) {
                    appendFeedbackWidget(box, data.recommendation_id, listId);
                }
            }
        } catch (err) {
            console.error(err);
            send.disabled = false;
            showError(t("feedback_error"));
        }
    });
    detail.append(comment, send);
    widget.appendChild(detail);
    box.appendChild(widget);
}

async function loadMoods() {
    const data = await fetchJson(`/api/moods?lang=${lang}`);
    const chips = $("mood-chips");
    chips.innerHTML = "";
    for (const m of data.moods) {
        const btn = document.createElement("button");
        btn.textContent = m.name;
        btn.dataset.slug = m.slug;
        btn.classList.toggle("selected", m.slug === mood);
        btn.addEventListener("click", () => {
            // Clicking the selected chip deselects it again (no mood bias).
            mood = mood === m.slug ? null : m.slug;
            if (mood) $("feel-input").value = "";  // the pick replaces free text
            chips.querySelectorAll("button").forEach((b) =>
                b.classList.toggle("selected", b === btn && mood !== null));
            renderContextChips();
        });
        chips.appendChild(btn);
    }
}

async function recommend() {
    const btn = $("recommend-btn");
    btn.disabled = true;
    btn.textContent = t("loading");
    $("error").classList.add("hidden");
    try {
        const useWardrobe = wardrobeCount > 0 && $("use-wardrobe").checked;
        const source = useWardrobe ? "wardrobe" : "catalog";
        const feeling = $("feel-input").value.trim();

        let data;
        if (feeling) {
            const res = await fetch("/api/feel", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(
                    { text: feeling, lang, source, ...weatherOptions() }),
            });
            data = await res.json();
            if (!res.ok) {
                showError(data.code === "no_api_key" ? t("ask_no_key") : t("error"));
                return;
            }
            $("mood-line").textContent = t("mood_line")(data.moods);
        }
        else {
            const moodParam = mood ? `&mood=${mood}` : "";
            data = await fetchJson(
                `/api/recommendation?lang=${lang}&source=${source}${moodParam}` +
                weatherQueryString());
        }
        $("mood-line").classList.toggle("hidden", !feeling);
        $("source-note").classList.toggle("hidden",
            !(useWardrobe && data.source === "catalog"));
        renderStage(data);
    } catch (err) {
        console.error(err);
        showError(t("error"));
    } finally {
        btn.disabled = false;
        btn.textContent = t("recommend");
    }
}

async function ask() {
    const text = $("ask-input").value.trim();
    if (!text) return;
    const btn = $("recommend-btn");
    btn.disabled = true;
    btn.textContent = t("asking");
    $("error").classList.add("hidden");
    try {
        const res = await fetch("/api/ask", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ text, lang, ...weatherOptions() }),
        });
        const data = await res.json();
        if (!res.ok) {
            showError(data.code === "no_api_key" ? t("ask_no_key") : t("error"));
            return;
        }
        $("mood-line").classList.add("hidden");
        $("source-note").classList.add("hidden");
        renderStage(data);
    } catch (err) {
        console.error(err);
        showError(t("error"));
    } finally {
        btn.disabled = false;
        btn.textContent = t("recommend");
    }
}

// One primary action: a written plan goes through the Gemini parser,
// otherwise the plain mood/weather recommendation runs.
function primaryAction() {
    if ($("ask-input").value.trim()) {
        ask();
    } else {
        recommend();
    }
}

let typeNames = {};  // type slug -> localized name, for the bulk progress list

async function loadTypes() {
    const data = await fetchJson(`/api/types?lang=${lang}`);
    typeNames = {};
    for (const type of data.types) typeNames[type.slug] = type.name;
    const select = $("type-select");
    select.innerHTML = "";
    const groups = new Map();
    for (const type of data.types) {
        if (!groups.has(type.category_slug)) {
            const group = document.createElement("optgroup");
            group.label = type.category_name;
            groups.set(type.category_slug, group);
            select.appendChild(group);
        }
        const option = document.createElement("option");
        option.value = type.slug;
        option.textContent = type.name;
        groups.get(type.category_slug).appendChild(option);
    }
    await loadAttributes();
}

// Multi-value attributes render as toggle chips instead of a single select.
const MULTI_ATTRIBUTES = ["color"];

// Selected value slugs per attribute slug, from both selects and chip groups.
function selectedAttributeValues() {
    const container = $("attribute-selects");
    const values = {};
    for (const select of container.querySelectorAll("select")) {
        if (select.value) values[select.dataset.attribute] = [select.value];
    }
    for (const group of container.querySelectorAll(".attr-chips")) {
        const picked = [...group.querySelectorAll("button.selected")].map(
            (b) => b.dataset.slug);
        if (picked.length > 0) values[group.dataset.attribute] = picked;
    }
    return values;
}

function setAttributeValues(values) {
    const container = $("attribute-selects");
    for (const select of container.querySelectorAll("select")) {
        const wanted = values[select.dataset.attribute];
        if (!wanted) continue;
        const slug = wanted[0];
        if ([...select.options].some((o) => o.value === slug)) select.value = slug;
    }
    for (const group of container.querySelectorAll(".attr-chips")) {
        const wanted = values[group.dataset.attribute] || [];
        for (const btn of group.querySelectorAll("button")) {
            btn.classList.toggle("selected", wanted.includes(btn.dataset.slug));
        }
    }
}

async function loadAttributes() {
    const type = $("type-select").value;
    const container = $("attribute-selects");
    // Rebuilding the controls (e.g. after the user corrects the type) must not
    // lose what is already picked — keep values for attributes that survive.
    const previous = selectedAttributeValues();
    container.innerHTML = "";
    if (!type) return;
    const data = await fetchJson(`/api/attributes?type=${type}&lang=${lang}`);
    for (const attr of data.attributes) {
        const label = document.createElement("label");
        const caption = document.createElement("span");
        caption.textContent = attr.name;
        if (MULTI_ATTRIBUTES.includes(attr.slug)) {
            const chips = document.createElement("div");
            chips.className = "chips attr-chips";
            chips.dataset.attribute = attr.slug;
            for (const v of attr.values) {
                const btn = document.createElement("button");
                btn.type = "button";
                btn.dataset.slug = v.slug;
                btn.textContent = v.name;
                btn.addEventListener("click", () =>
                    btn.classList.toggle("selected"));
                chips.appendChild(btn);
            }
            label.append(caption, chips);
        } else {
            const select = document.createElement("select");
            select.dataset.attribute = attr.slug;
            const none = document.createElement("option");
            none.value = "";
            none.textContent = t("choose");
            select.appendChild(none);
            for (const v of attr.values) {
                const option = document.createElement("option");
                option.value = v.slug;
                option.textContent = v.name;
                select.appendChild(option);
            }
            label.append(caption, select);
        }
        container.appendChild(label);
    }
    setAttributeValues(previous);
}

// Fixed display order for the wardrobe categories.
const CATEGORY_ORDER =
    ["outerwear", "top", "bottom", "one-piece", "footwear", "accessory", "jewelry"];

let wardrobeItems = [];
let wardrobeFilter = "all";   // a category slug, "all", "outfits" or "analytics"
let extractRunning = false;
let savedOutfits = null;      // cache for the OUTFITS tab; null = not loaded
let analyticsCache = null;    // cache for the ANALYSIS tab; null = not loaded

// Swatch colors for the analytics palette bars.
const COLOR_HEX = {
    black: "#1f1f24", white: "#f4f4f2", gray: "#9aa0a6", charcoal: "#3c4043",
    navy: "#1f3a5f", blue: "#3b82f6", red: "#dc2626", green: "#16a34a",
    yellow: "#eab308", pink: "#ec4899", purple: "#8b5cf6", orange: "#f97316",
    brown: "#8b5e34", beige: "#d9c7a7", cream: "#efe3cd", khaki: "#b0a06a",
    turquoise: "#14b8a6", olive: "#708238", burgundy: "#800020",
    mustard: "#d4a017", teal: "#0d9488", lilac: "#c8a2c8", mint: "#98e4c0",
    coral: "#ff7f6b", silver: "#c8ccd2", gold: "#d4af37",
};

async function loadWardrobe() {
    const data = await fetchJson(`/api/wardrobe?lang=${lang}`);
    savedOutfits = null;   // items or language changed; refetch on demand
    analyticsCache = null;
    wardrobeItems = data.items;
    wardrobeCount = data.items.length;
    $("wardrobe-empty").classList.toggle("hidden", wardrobeCount > 0);
    $("wardrobe-toggle").classList.toggle("hidden", wardrobeCount === 0);
    $("wardrobe-count").textContent = wardrobeCount;
    $("wardrobe-count").classList.toggle("hidden", wardrobeCount === 0);
    renderWardrobe();
}

// One flat gallery filtered by category tabs, like a lookbook; the last tab
// shows saved outfits instead of garments.
function renderWardrobe() {
    const present = CATEGORY_ORDER.filter(
        (slug) => wardrobeItems.some((item) => item.category_slug === slug));
    if (!["all", "outfits", "analytics"].includes(wardrobeFilter) &&
        !present.includes(wardrobeFilter)) {
        wardrobeFilter = "all";
    }

    const filter = $("wardrobe-filter");
    filter.innerHTML = "";
    const addTab = (slug, name, extraClass) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.textContent = name;
        if (extraClass) btn.classList.add(extraClass);
        btn.classList.toggle("active", wardrobeFilter === slug);
        btn.addEventListener("click", () => {
            wardrobeFilter = slug;
            closePanel();
            renderWardrobe();
        });
        filter.appendChild(btn);
    };
    addTab("all", t("filter_all"));
    for (const slug of present) {
        const item = wardrobeItems.find((i) => i.category_slug === slug);
        addTab(slug, item.category_name);
    }
    addTab("outfits", t("outfits_tab"), "outfits-tab");
    addTab("analytics", t("analytics_tab"));

    const showOutfits = wardrobeFilter === "outfits";
    const showAnalytics = wardrobeFilter === "analytics";
    $("wardrobe-gallery").classList.toggle("hidden", showOutfits || showAnalytics);
    $("outfits-gallery").classList.toggle("hidden", !showOutfits);
    $("analytics-panel").classList.toggle("hidden", !showAnalytics);
    if (showOutfits) {
        renderOutfits();
    } else if (showAnalytics) {
        renderAnalytics();
    } else {
        renderGallery();
    }
    renderExtractBar();
}

// ---- the ANALYSIS tab: distribution, wear stats and gaps ----

async function renderAnalytics() {
    const panel = $("analytics-panel");
    if (analyticsCache === null) {
        try {
            analyticsCache = await fetchJson(`/api/analytics?lang=${lang}`);
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
            return;
        }
    }
    const a = analyticsCache;
    $("lookbook-count").textContent = t("items_count")(a.total);
    panel.innerHTML = "";

    const card = (titleKey) => {
        const div = document.createElement("div");
        div.className = "an-card";
        if (titleKey) {
            const h = document.createElement("h3");
            h.className = "an-title";
            h.textContent = t(titleKey);
            div.appendChild(h);
        }
        panel.appendChild(div);
        return div;
    };

    // Stat tiles.
    const stats = card(null);
    stats.classList.add("an-stats");
    for (const [value, key] of [[a.total, "an_total"], [a.dirty, "an_dirty"],
                                [a.never_worn, "an_never"],
                                [a.with_cutout, "an_cutout"]]) {
        const tile = document.createElement("div");
        tile.className = "stat-tile";
        const num = document.createElement("strong");
        num.textContent = value;
        const label = document.createElement("span");
        label.textContent = t(key);
        tile.append(num, label);
        stats.appendChild(tile);
    }

    // Category distribution bars.
    const cats = card("an_categories");
    const maxCat = Math.max(1, ...a.categories.map((c) => c.count));
    for (const c of a.categories) {
        cats.appendChild(barRow(c.name, c.count, c.count / maxCat, "var(--accent)"));
    }

    // Color palette bars with real swatch colors.
    const cols = card("an_colors");
    const maxColor = Math.max(1, ...a.colors.map((c) => c.count));
    for (const c of a.colors) {
        cols.appendChild(barRow(c.name, c.count, c.count / maxColor,
                                COLOR_HEX[c.slug] || "#9aa0a6"));
    }

    // Most worn pieces.
    const worn = card("an_most_worn");
    if (a.most_worn.length === 0) {
        const note = document.createElement("p");
        note.className = "note";
        note.textContent = t("an_no_wears");
        worn.appendChild(note);
    }
    const maxWears = Math.max(1, ...a.most_worn.map((m) => m.wears));
    for (const m of a.most_worn) {
        worn.appendChild(barRow(m.name, t("wears_unit")(m.wears),
                                m.wears / maxWears, "var(--ink)"));
    }

    // Gaps.
    const gaps = card("an_gaps");
    if (a.gaps.length === 0) {
        const note = document.createElement("p");
        note.className = "note";
        note.textContent = t("an_no_gaps");
        gaps.appendChild(note);
    } else {
        const list = document.createElement("div");
        list.className = "detail-tags";
        const categoryNames = {};
        for (const item of wardrobeItems) {
            categoryNames[item.category_slug] = item.category_name;
        }
        for (const gap of a.gaps) {
            const tag = document.createElement("span");
            tag.textContent =
                t("gap_line")(categoryNames[gap.category] || gap.category, gap.band);
            list.appendChild(tag);
        }
        gaps.appendChild(list);
    }
}

function barRow(label, value, fraction, color) {
    const row = document.createElement("div");
    row.className = "bar-row";
    const name = document.createElement("span");
    name.className = "bar-label";
    name.textContent = label;
    const track = document.createElement("span");
    track.className = "bar-track";
    const fill = document.createElement("span");
    fill.style.width = `${Math.round(fraction * 100)}%`;
    fill.style.background = color;
    track.appendChild(fill);
    const count = document.createElement("span");
    count.className = "bar-value";
    count.textContent = value;
    row.append(name, track, count);
    return row;
}

function galleryItems() {
    return wardrobeItems.filter(
        (item) => wardrobeFilter === "all" || wardrobeFilter === "outfits" ||
                  item.category_slug === wardrobeFilter);
}

function renderGallery() {
    const gallery = $("wardrobe-gallery");
    gallery.innerHTML = "";
    const items = galleryItems();
    for (const item of items) {
        gallery.appendChild(wardrobeCard(item));
    }
    $("lookbook-count").textContent = t("items_count")(items.length);
}

function itemsMissingCutout() {
    return wardrobeItems.filter((item) => item.photo_url && !item.cutout_url);
}

function renderExtractBar() {
    const missing = itemsMissingCutout();
    const btn = $("extract-all-btn");
    btn.textContent = t("extract_all")(missing.length);
    btn.disabled = extractRunning;
    btn.classList.toggle("hidden",
        missing.length === 0 || wardrobeFilter === "outfits" ||
        wardrobeFilter === "analytics");
    if (!extractRunning) $("extract-status").textContent = "";
    // The builder belongs to the OUTFITS view; photo upload to the rest.
    $("create-outfit-btn").classList.toggle("hidden", wardrobeFilter !== "outfits");
    $("add-btn").classList.toggle("hidden",
        wardrobeFilter === "outfits" || wardrobeFilter === "analytics");
}

// Sends every photo that has no cutout yet through the extraction endpoint,
// one at a time. The first item is slow (the background-removal model loads
// once); after that it is fast and free — everything runs locally.
async function extractAllMissing() {
    if (extractRunning) return;
    const missing = itemsMissingCutout();
    if (missing.length === 0) return;
    extractRunning = true;
    const status = $("extract-status");
    let failed = 0;
    let stopReason = "";
    for (let i = 0; i < missing.length; ++i) {
        renderExtractBar();
        status.textContent = t("extracting")(i + 1, missing.length);
        try {
            const res = await fetch(`/api/wardrobe/${missing[i].id}/extract`,
                                    { method: "POST" });
            const data = await res.json();
            if (!res.ok) {
                failed += 1;
                if (data.code === "rate_limited") {
                    stopReason = "extract_rate_limited";
                    break;
                }
                if (data.code === "no_local_extractor") {
                    stopReason = "extract_no_tool";
                    break;
                }
                continue;
            }
            const item = wardrobeItems.find((w) => w.id === missing[i].id);
            if (item) item.cutout_url = data.cutout_url;
            renderGallery();
        } catch (err) {
            console.error(err);
            failed += 1;
        }
    }
    extractRunning = false;
    renderExtractBar();
    status.textContent = stopReason ? t(stopReason)
        : failed > 0 ? t("extract_failed")(failed)
        : t("extract_done");
}

function wardrobeCard(item) {
    const li = document.createElement("li");

    const visual = document.createElement("div");
    visual.className = "card-visual";
    if (item.is_dirty) visual.classList.add("dirty");
    if (item.cutout_url || item.photo_url) {
        visual.classList.add(item.cutout_url ? "cutout" : "photo");
        const img = document.createElement("img");
        img.src = item.cutout_url || item.photo_url;
        img.alt = item.label || item.type_name;
        img.loading = "lazy";
        visual.appendChild(img);
    } else {
        visual.classList.add("emoji");
        visual.textContent = CATEGORY_EMOJI[item.category_slug] || "👔";
    }
    if (item.is_dirty) {
        const badge = document.createElement("span");
        badge.className = "dirty-badge";
        badge.textContent = "🧺";
        badge.title = t("dirty_note");
        li.appendChild(badge);
    }

    const name = document.createElement("span");
    name.className = "card-name";
    name.textContent = item.label || item.type_name;

    const del = document.createElement("button");
    del.className = "delete-btn";
    del.textContent = "✕";
    del.title = t("delete");
    del.addEventListener("click", async (e) => {
        e.stopPropagation();
        try {
            await fetchJson(`/api/wardrobe/${item.id}`, { method: "DELETE" });
            closePanel();
            await loadWardrobe();
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
        }
    });

    li.addEventListener("click", () => openItemPanel(item));
    li.append(visual, name, del);
    return li;
}

// ---- detail slide-in panel ----

function closePanel() {
    $("detail-panel").classList.add("hidden");
    $("panel-backdrop").classList.add("hidden");
}

function openPanel() {
    const panel = $("detail-panel");
    panel.innerHTML = "";
    panel.classList.remove("hidden");
    $("panel-backdrop").classList.remove("hidden");
    panel.scrollTop = 0;
    return panel;
}

function panelHead(panel, kickerText, plain = false) {
    const head = document.createElement("div");
    head.className = "panel-head";
    const kicker = document.createElement("span");
    kicker.className = plain ? "panel-kicker plain" : "panel-kicker";
    kicker.textContent = kickerText;
    const close = document.createElement("button");
    close.className = "panel-close";
    close.textContent = "✕";
    close.addEventListener("click", closePanel);
    head.append(kicker, close);
    panel.appendChild(head);
    return head;
}

function panelSection(panel, labelText) {
    const section = document.createElement("div");
    section.className = "panel-section";
    if (labelText) {
        const label = document.createElement("span");
        label.className = "panel-label";
        label.textContent = labelText;
        section.appendChild(label);
    }
    panel.appendChild(section);
    return section;
}

// Dominant color and a small palette, sampled from the item's image on a
// canvas. Transparent pixels are skipped, so cutouts yield garment colors.
function computePalette(url) {
    return new Promise((resolve) => {
        const img = new Image();
        img.onload = () => {
            const size = 48;
            const canvas = document.createElement("canvas");
            canvas.width = size;
            canvas.height = size;
            const g = canvas.getContext("2d");
            g.drawImage(img, 0, 0, size, size);
            const data = g.getImageData(0, 0, size, size).data;
            const buckets = new Map();
            for (let i = 0; i < data.length; i += 4) {
                if (data[i + 3] < 128) continue;  // transparent
                // Quantize to 32-steps so shades group together.
                const key = [data[i], data[i + 1], data[i + 2]]
                    .map((v) => Math.min(224, Math.round(v / 32) * 32)).join(",");
                buckets.set(key, (buckets.get(key) || 0) + 1);
            }
            const top = [...buckets.entries()]
                .sort((a, b) => b[1] - a[1])
                .slice(0, 5)
                .map(([key]) => {
                    const [r, g2, b] = key.split(",").map(Number);
                    return "#" + [r, g2, b]
                        .map((v) => v.toString(16).padStart(2, "0")).join("")
                        .toUpperCase();
                });
            resolve(top);
        };
        img.onerror = () => resolve([]);
        img.src = url;
    });
}

async function openItemPanel(item) {
    const panel = openPanel();
    panelHead(panel, item.category_name);

    const hero = document.createElement("div");
    hero.className = "panel-hero";
    if (item.cutout_url || item.photo_url) {
        const img = document.createElement("img");
        img.src = item.cutout_url || item.photo_url;
        img.alt = item.label || item.type_name;
        hero.appendChild(img);
    } else {
        const emoji = document.createElement("span");
        emoji.className = "emoji";
        emoji.textContent = CATEGORY_EMOJI[item.category_slug] || "👔";
        hero.appendChild(emoji);
    }
    panel.appendChild(hero);

    // NAME (editable, saved on change) + CATEGORY side by side.
    const row = document.createElement("div");
    row.className = "panel-row";
    const nameCell = document.createElement("div");
    const nameLabel = document.createElement("span");
    nameLabel.className = "panel-label";
    nameLabel.textContent = t("panel_name");
    const nameInput = document.createElement("input");
    nameInput.type = "text";
    nameInput.maxLength = 60;
    nameInput.value = item.label || "";
    nameInput.placeholder = item.type_name;
    nameInput.addEventListener("change", async () => {
        try {
            await fetchJson(`/api/wardrobe/${item.id}`, {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ label: nameInput.value.trim() }),
            });
            item.label = nameInput.value.trim();
            nameLabel.textContent = `${t("panel_name")} — ${t("name_saved")}`;
            renderGallery();
        } catch (err) {
            console.error(err);
            showError(t("save_error"), "wardrobe-error");
        }
    });
    nameCell.append(nameLabel, nameInput);
    const catCell = document.createElement("div");
    const catLabel = document.createElement("span");
    catLabel.className = "panel-label";
    catLabel.textContent = t("panel_category");
    const catValue = document.createElement("div");
    catValue.className = "panel-static";
    catValue.textContent = item.category_name;
    catCell.append(catLabel, catValue);
    row.append(nameCell, catCell);
    const rowSection = panelSection(panel, "");
    rowSection.appendChild(row);

    // COLORS: dominant + palette computed from the image.
    if (item.cutout_url || item.photo_url) {
        const colors = panelSection(panel, t("panel_colors"));
        const palette = await computePalette(item.cutout_url || item.photo_url);
        if (palette.length > 0) {
            const primary = document.createElement("div");
            primary.className = "swatch-row";
            const main = document.createElement("span");
            main.className = "swatch";
            main.style.background = palette[0];
            const hex = document.createElement("span");
            hex.innerHTML = `<span class="swatch-caption">${t("panel_primary")}</span><br>` +
                            `<span class="swatch-hex">${palette[0]}</span>`;
            primary.append(main, hex);
            colors.appendChild(primary);
            if (palette.length > 1) {
                const caption = document.createElement("span");
                caption.className = "swatch-caption";
                caption.style.display = "block";
                caption.style.margin = "0.6rem 0 0.3rem";
                caption.textContent = t("panel_palette");
                colors.appendChild(caption);
                const rowEl = document.createElement("div");
                rowEl.className = "swatch-row";
                for (const color of palette.slice(1)) {
                    const s = document.createElement("span");
                    s.className = "swatch small";
                    s.style.background = color;
                    s.title = color;
                    rowEl.appendChild(s);
                }
                colors.appendChild(rowEl);
            }
        }
    }

    // DETAILS: the item's recorded attribute values as tags.
    const details = panelSection(panel, t("panel_details"));
    if (item.values.length > 0) {
        const tags = document.createElement("div");
        tags.className = "detail-tags";
        for (const v of item.values) {
            const tag = document.createElement("span");
            tag.textContent = v.name;
            tags.appendChild(tag);
        }
        details.appendChild(tags);
    } else {
        const none = document.createElement("p");
        none.className = "note";
        none.textContent = t("panel_no_details");
        details.appendChild(none);
    }

    // Laundry: wear info + a dirty/clean toggle.
    const laundry = panelSection(panel, "");
    const wornLine = document.createElement("p");
    wornLine.className = "note";
    wornLine.textContent = item.last_worn_at
        ? t("last_worn")(item.last_worn_at.slice(0, 10))
        : t("never_worn");
    laundry.appendChild(wornLine);
    if (item.is_dirty) {
        const dirtyNote = document.createElement("p");
        dirtyNote.className = "note";
        dirtyNote.textContent = `🧺 ${t("dirty_note")}`;
        laundry.appendChild(dirtyNote);
    }
    const laundryBtn = document.createElement("button");
    laundryBtn.className = "ghost-btn";
    laundryBtn.textContent = item.is_dirty ? t("mark_clean") : t("mark_dirty");
    laundryBtn.addEventListener("click", async () => {
        try {
            await fetchJson(`/api/wardrobe/${item.id}/laundry`, {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ dirty: !item.is_dirty }),
            });
            item.is_dirty = !item.is_dirty;
            await loadWardrobe();
            openItemPanel(item);  // repaint the panel with the new state
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
        }
    });
    laundry.appendChild(laundryBtn);

    const danger = document.createElement("div");
    danger.className = "panel-danger";
    const del = document.createElement("button");
    del.className = "ghost-btn";
    del.textContent = `✕ ${t("delete")}`;
    del.addEventListener("click", async () => {
        try {
            await fetchJson(`/api/wardrobe/${item.id}`, { method: "DELETE" });
            closePanel();
            await loadWardrobe();
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
        }
    });
    danger.appendChild(del);
    panel.appendChild(danger);
}

// ---- the OUTFITS tab ----

async function renderOutfits() {
    const grid = $("outfits-gallery");
    if (savedOutfits === null) {
        grid.innerHTML = "";
        try {
            const data = await fetchJson(`/api/outfits?lang=${lang}`);
            savedOutfits = data.outfits;
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
            return;
        }
    }
    grid.innerHTML = "";
    $("lookbook-count").textContent = t("outfits_count")(savedOutfits.length);
    if (savedOutfits.length === 0) {
        const empty = document.createElement("p");
        empty.className = "note";
        empty.textContent = t("no_outfits");
        grid.appendChild(empty);
        return;
    }
    savedOutfits.forEach((outfit, index) => {
        const number = savedOutfits.length - index;  // newest first, highest number
        const tile = document.createElement("div");
        tile.className = "look-tile";
        const visual = document.createElement("div");
        visual.className = "look-tile-visual";
        const withImage = outfit.items.filter((i) => i.cutout_url || i.photo_url);
        if (withImage.length > 0) {
            for (const item of withImage.slice(0, 4)) {
                const img = document.createElement("img");
                img.src = item.cutout_url || item.photo_url;
                img.alt = item.item_name;
                img.loading = "lazy";
                visual.appendChild(img);
            }
        } else {
            const emoji = document.createElement("span");
            emoji.className = "emoji";
            emoji.textContent = outfit.items
                .map((i) => CATEGORY_EMOJI[i.category_slug] || "👔").join(" ");
            visual.appendChild(emoji);
        }
        const label = document.createElement("span");
        label.className = "look-tile-label";
        label.textContent = outfit.title || `${t("look_word")} ${number}`;

        const del = document.createElement("button");
        del.className = "delete-btn";
        del.textContent = "✕";
        del.title = t("delete_outfit");
        del.addEventListener("click", (e) => {
            e.stopPropagation();
            deleteOutfit(outfit.id);
        });

        tile.append(visual, label, del);
        tile.addEventListener("click", () => openOutfitPanel(outfit, number));
        grid.appendChild(tile);
    });
}

async function deleteOutfit(id) {
    try {
        await fetchJson(`/api/recommendations/${id}`, { method: "DELETE" });
        savedOutfits = null;
        closePanel();
        renderWardrobe();
    } catch (err) {
        console.error(err);
        showError(t("error"), "wardrobe-error");
    }
}

function openOutfitPanel(outfit, number) {
    const panel = openPanel();
    panelHead(panel, outfit.title || `${t("look_word")} ${number}`, /*plain=*/true);

    const board = buildLookBoard(outfit.items);
    if (board) {
        panel.appendChild(board);
    } else {
        const list = document.createElement("ul");
        list.className = "outfit";
        for (const item of outfit.items) {
            const li = document.createElement("li");
            const text = document.createElement("span");
            const category = document.createElement("span");
            category.className = "category";
            category.textContent = item.category_slug;
            const name = document.createElement("span");
            name.textContent = item.item_name;
            text.append(category, name);
            li.appendChild(text);
            list.appendChild(li);
        }
        panel.appendChild(list);
    }

    const title = document.createElement("h3");
    title.className = "panel-title";
    title.textContent = outfit.items.map((i) => i.item_name).slice(0, 3).join(" · ");
    panel.appendChild(title);

    const desc = document.createElement("p");
    desc.className = "panel-text";
    desc.textContent = outfit.explanation || t("outfit_desc_fallback");
    panel.appendChild(desc);

    const tags = document.createElement("div");
    tags.className = "detail-tags";
    tags.style.marginTop = "0.9rem";
    if (outfit.mood_name) {
        const mood = document.createElement("span");
        mood.textContent = outfit.mood_name;
        tags.appendChild(mood);
    }
    for (const item of outfit.items.slice(0, 3)) {
        if (!item.category_slug) continue;
        const tag = document.createElement("span");
        tag.textContent = item.category_slug;
        tags.appendChild(tag);
    }
    panel.appendChild(tags);

    const danger = document.createElement("div");
    danger.className = "panel-danger";
    const del = document.createElement("button");
    del.className = "ghost-btn";
    del.textContent = t("delete_outfit");
    del.addEventListener("click", () => deleteOutfit(outfit.id));
    danger.appendChild(del);
    panel.appendChild(danger);
}

// ---- the outfit builder: compose and name your own look ----

function openBuilderPanel() {
    const panel = openPanel();
    panelHead(panel, t("create_outfit"));

    const nameSection = panelSection(panel, t("builder_name"));
    const nameInput = document.createElement("input");
    nameInput.type = "text";
    nameInput.maxLength = 60;
    nameInput.style.width = "100%";
    nameSection.appendChild(nameInput);

    // Live preview of the picked pieces, composed like a look board.
    const preview = document.createElement("div");
    preview.className = "look-board builder-preview";
    panel.appendChild(preview);

    const selected = new Set();
    const refreshPreview = () => {
        const items = wardrobeItems.filter((i) => selected.has(i.id)).map((i) => ({
            category_slug: i.category_slug,
            item_name: i.label || i.type_name,
            cutout_url: i.cutout_url,
            photo_url: i.photo_url,
        }));
        preview.innerHTML = "";
        const board = buildLookBoard(items);
        if (board) {
            while (board.firstChild) preview.appendChild(board.firstChild);
        }
        preview.classList.toggle("hidden", !preview.firstChild);
        save.disabled = selected.size < 2;
    };

    const pickSection = panelSection(panel, t("builder_pick"));
    for (const slug of CATEGORY_ORDER) {
        const items = wardrobeItems.filter((i) => i.category_slug === slug);
        if (items.length === 0) continue;
        const caption = document.createElement("span");
        caption.className = "swatch-caption";
        caption.style.display = "block";
        caption.style.margin = "0.5rem 0 0.25rem";
        caption.textContent = items[0].category_name;
        pickSection.appendChild(caption);
        const row = document.createElement("div");
        row.className = "builder-grid";
        for (const item of items) {
            const cell = document.createElement("button");
            cell.type = "button";
            cell.className = "builder-cell";
            if (item.cutout_url || item.photo_url) {
                const img = document.createElement("img");
                img.src = item.cutout_url || item.photo_url;
                img.alt = item.label || item.type_name;
                img.loading = "lazy";
                cell.appendChild(img);
            } else {
                cell.textContent = CATEGORY_EMOJI[slug] || "👔";
            }
            cell.title = item.label || item.type_name;
            cell.addEventListener("click", () => {
                if (selected.has(item.id)) {
                    selected.delete(item.id);
                } else {
                    selected.add(item.id);
                }
                cell.classList.toggle("selected", selected.has(item.id));
                refreshPreview();
            });
            row.appendChild(cell);
        }
        pickSection.appendChild(row);
    }

    const save = document.createElement("button");
    save.className = "btn";
    save.textContent = t("builder_save");
    save.disabled = true;
    save.title = t("builder_need");
    save.addEventListener("click", async () => {
        save.disabled = true;
        try {
            await fetchJson("/api/outfits", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    title: nameInput.value.trim(),
                    wardrobe_ids: [...selected],
                }),
            });
            savedOutfits = null;
            closePanel();
            wardrobeFilter = "outfits";
            renderWardrobe();
        } catch (err) {
            console.error(err);
            save.disabled = false;
            showError(t("save_error"), "wardrobe-error");
        }
    });
    panel.appendChild(save);
}

// Classifies and saves one photo; updates its progress row. Returns true
// when the item landed in the wardrobe.
async function processBulkFile(original, li, status) {
    li.classList.remove("done", "failed");
    status.textContent = t("bulk_reading");
    try {
        const file = await shrinkPhoto(original);
        const res = await fetch("/api/classify-photo", {
            method: "POST",
            headers: { "Content-Type": file.type },
            body: file,
        });
        const classified = await res.json();
        if (!res.ok) {
            status.textContent =
                classified.code === "no_api_key" ? t("ask_no_key")
                : classified.code === "rate_limited" ? t("bulk_rate_limited")
                : t("bulk_failed");
            li.classList.add("failed");
            offerRetry(original, li, status);
            return false;
        }
        const created = await fetchJson("/api/wardrobe", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                type: classified.type,
                label: "",
                values: Object.values(classified.values).flat(),
            }),
        });
        await fetchJson(`/api/wardrobe/${created.id}/photo`, {
            method: "PUT",
            headers: { "Content-Type": file.type },
            body: file,
        });
        status.textContent =
            `✓ ${typeNames[classified.type] || classified.type} ${t("bulk_saved")}`;
        li.classList.add("done");
        li.querySelector(".retry-btn")?.remove();
        return true;
    } catch (err) {
        console.error(err);
        status.textContent = t("bulk_failed");
        li.classList.add("failed");
        offerRetry(original, li, status);
        return false;
    }
}

// A failed row gets a retry button; the file stays in memory, so the user
// never has to re-pick their photos.
function offerRetry(original, li, status) {
    if (li.querySelector(".retry-btn")) return;
    const retry = document.createElement("button");
    retry.type = "button";
    retry.className = "link-btn retry-btn";
    retry.textContent = t("bulk_retry");
    retry.addEventListener("click", async () => {
        retry.disabled = true;
        const ok = await processBulkFile(original, li, status);
        retry.disabled = false;
        if (ok) await loadWardrobe();
    });
    li.appendChild(retry);
}

// Bulk add: classify and save each selected photo, reporting per-file
// progress. Files are processed one by one to keep the API load gentle.
async function bulkUpload() {
    const files = [...$("bulk-input").files];
    if (files.length === 0) return;
    $("bulk-input").value = "";
    const progress = $("bulk-progress");
    progress.innerHTML = "";

    const rows = files.map((original) => {
        const li = document.createElement("li");
        const label = document.createElement("span");
        label.textContent = original.name;
        const status = document.createElement("span");
        status.className = "bulk-status";
        status.textContent = "…";
        li.append(label, status);
        progress.appendChild(li);
        return { original, li, status };
    });

    let added = 0;
    for (const row of rows) {
        if (await processBulkFile(row.original, row.li, row.status)) added += 1;
    }

    const summary = document.createElement("li");
    summary.className = "bulk-summary";
    summary.textContent = t("bulk_summary")(added, files.length);
    progress.appendChild(summary);
    await loadWardrobe();
    // Fresh photos go straight to background cleanup — that is the point of
    // uploading them. Existing items only ever start from the button.
    if (added > 0) await extractAllMissing();
}

async function classifyPhoto() {
    const original = $("photo-input").files[0];
    const note = $("classify-note");
    if (!original) {
        note.classList.add("hidden");
        return;
    }
    note.textContent = t("classifying");
    note.classList.remove("hidden");
    try {
        const file = await shrinkPhoto(original);
        const res = await fetch("/api/classify-photo", {
            method: "POST",
            headers: { "Content-Type": file.type },
            body: file,
        });
        const data = await res.json();
        if (!res.ok) {
            note.textContent = data.code === "no_api_key" ? t("ask_no_key") : t("classify_error");
            return;
        }
        $("type-select").value = data.type;
        await loadAttributes();
        setAttributeValues(data.values);  // every value is an array of slugs
        note.textContent = t("classified");
    } catch (err) {
        console.error(err);
        note.textContent = t("classify_error");
    }
}

async function saveItem() {
    const btn = $("save-btn");
    btn.disabled = true;
    btn.textContent = t("saving");
    $("wardrobe-error").classList.add("hidden");
    try {
        const values = Object.values(selectedAttributeValues()).flat();
        const body = {
            type: $("type-select").value,
            label: $("label-input").value.trim(),
            values,
        };
        const created = await fetchJson("/api/wardrobe", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body),
        });

        const original = $("photo-input").files[0];
        if (original) {
            const file = await shrinkPhoto(original);
            await fetchJson(`/api/wardrobe/${created.id}/photo`, {
                method: "PUT",
                headers: { "Content-Type": file.type },
                body: file,
            });
        }

        $("label-input").value = "";
        $("photo-input").value = "";
        $("classify-note").classList.add("hidden");
        await loadWardrobe();
        if (original) await extractAllMissing();
    } catch (err) {
        console.error(err);
        showError(t("save_error"), "wardrobe-error");
    } finally {
        btn.disabled = false;
        btn.textContent = t("save");
    }
}

async function refreshAll() {
    applyStaticStrings();
    await Promise.all([loadMoods(), loadTypes(), loadWardrobe()]);
}

document.querySelectorAll(".lang-switch button").forEach((btn) => {
    btn.addEventListener("click", async () => {
        lang = btn.dataset.lang;
        localStorage.setItem("lang", lang);
        await refreshAll();
        renderContextChips();
        if ($("stage-empty").classList.contains("hidden")) primaryAction();
    });
});

function showTab(name) {
    document.querySelectorAll(".tabs .tab").forEach((btn) =>
        btn.classList.toggle("active", btn.dataset.tab === name));
    $("tab-home").classList.toggle("hidden", name !== "home");
    $("tab-wardrobe").classList.toggle("hidden", name !== "wardrobe");
    localStorage.setItem("tab", name);
}

document.querySelectorAll(".tabs .tab").forEach((btn) => {
    btn.addEventListener("click", () => showTab(btn.dataset.tab));
});
showTab(localStorage.getItem("tab") || "home");

$("bulk-input").addEventListener("change", bulkUpload);
$("extract-all-btn").addEventListener("click", extractAllMissing);
function toggleAddPanel() {
    const panel = $("add-panel");
    panel.classList.toggle("hidden");
    if (!panel.classList.contains("hidden")) {
        panel.scrollIntoView({ behavior: "smooth", block: "start" });
    }
}
$("add-toggle-btn").addEventListener("click", toggleAddPanel);
$("add-btn").addEventListener("click", toggleAddPanel);
$("create-outfit-btn").addEventListener("click", openBuilderPanel);
$("panel-backdrop").addEventListener("click", closePanel);
document.addEventListener("keydown", (e) => {
    if (e.key === "Escape") closePanel();
});

$("recommend-btn").addEventListener("click", primaryAction);
$("again-btn").addEventListener("click", primaryAction);
$("ask-input").addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
        e.preventDefault();
        primaryAction();
    }
});
$("context-edit-btn").addEventListener("click", toggleContextEditor);
$("type-select").addEventListener("change", loadAttributes);
$("photo-input").addEventListener("change", classifyPhoto);
$("save-btn").addEventListener("click", saveItem);

$("location-edit-btn").addEventListener("click", () => {
    $("location-editor").classList.toggle("hidden");
    $("city-input").focus();
});
$("city-input").addEventListener("input", () => {
    clearTimeout(citySearchTimer);
    citySearchTimer = setTimeout(searchCity, 400);
});
$("location-auto-btn").addEventListener("click", async () => {
    savedLocation = null;
    localStorage.removeItem("location");
    $("location-editor").classList.add("hidden");
    $("city-input").value = "";
    $("city-results").innerHTML = "";
    await loadLocation();
});
$("manual-weather").addEventListener("change", () => {
    $("manual-weather-fields").classList.toggle(
        "hidden", !$("manual-weather").checked);
    renderContextChips();
});
$("manual-temp").addEventListener("change", renderContextChips);
$("gender-select").value = localStorage.getItem("gender") || "";
$("gender-select").addEventListener("change", () => {
    localStorage.setItem("gender", $("gender-select").value);
    renderContextChips();
});
for (const id of ["hour-start", "hour-end"]) {
    $(id).addEventListener("change", renderContextChips);
}
$("feel-input").addEventListener("change", renderContextChips);

initHourSelects();
renderContextChips();
renderRecentLooks();
refreshAll().catch((err) => {
    console.error(err);
    showError(STRINGS[lang].error);
});
loadLocation();
