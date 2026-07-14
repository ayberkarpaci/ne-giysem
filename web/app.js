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
        ask_title: "Tell me your plan",
        ask_placeholder: "e.g. going to work tomorrow, smart but comfy",
        ask: "Suggest an outfit",
        asking: "Thinking...",
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
        ask_title: "Planını anlat",
        ask_placeholder: "örn. yarın işe gideceğim, şık ama rahat olsun",
        ask: "Kombin öner",
        asking: "Düşünüyorum...",
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
    },
};

const CATEGORY_EMOJI = {
    outerwear: "🧥",
    top: "👕",
    bottom: "👖",
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
    renderLocationName();
}

function renderLocationName() {
    $("location-name").textContent =
        savedLocation ? savedLocation.city
                      : detectedCity || t("location_unknown");
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

function renderOutfitList(listEl, items) {
    listEl.innerHTML = "";
    for (const item of items) {
        const li = document.createElement("li");
        const visual = document.createElement("span");
        if (item.photo_url) {
            const img = document.createElement("img");
            img.src = item.photo_url;
            img.alt = item.item_name;
            img.className = "thumb";
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
            detail.classList.remove("hidden");
        });
        starButtons.push(btn);
        stars.appendChild(btn);
    }
    widget.appendChild(stars);

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
                    lang,
                    gender: $("gender-select").value || undefined,
                }),
            });
            box.innerHTML = "";
            box.appendChild(note("feedback_thanks"));
            if (data.outfit) {
                box.appendChild(note("revised_note"));
                renderOutfitList($(listId), data.outfit);
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

        $("weather-line").textContent = t("weather")(data.weather);
        $("source-note").classList.toggle("hidden",
            !(useWardrobe && data.source === "catalog"));

        renderOutfitList($("outfit-list"), data.outfit);
        $("explanation").textContent = data.explanation || "";
        $("explanation").classList.toggle("hidden", !data.explanation);
        renderFeedback("feedback-box", data.recommendation_id, "outfit-list");
        $("result").classList.remove("hidden");
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
    const btn = $("ask-btn");
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
        $("ask-weather-line").textContent = t("weather")(data.weather);
        renderOutfitList($("ask-outfit-list"), data.outfit);
        $("ask-explanation").textContent = data.explanation || "";
        renderFeedback("ask-feedback-box", data.recommendation_id, "ask-outfit-list");
        $("ask-result").classList.remove("hidden");
    } catch (err) {
        console.error(err);
        showError(t("error"));
    } finally {
        btn.disabled = false;
        btn.textContent = t("ask");
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

// Fixed display order for the wardrobe groups.
const CATEGORY_ORDER = ["outerwear", "top", "bottom", "footwear", "accessory", "jewelry"];

async function loadWardrobe() {
    const data = await fetchJson(`/api/wardrobe?lang=${lang}`);
    wardrobeCount = data.items.length;
    $("wardrobe-empty").classList.toggle("hidden", wardrobeCount > 0);
    $("wardrobe-toggle").classList.toggle("hidden", wardrobeCount === 0);
    $("wardrobe-count").textContent = wardrobeCount;
    $("wardrobe-count").classList.toggle("hidden", wardrobeCount === 0);

    // Group the items by category, in a fixed category order.
    const groups = new Map();
    for (const item of data.items) {
        if (!groups.has(item.category_slug)) {
            groups.set(item.category_slug, { name: item.category_name, items: [] });
        }
        groups.get(item.category_slug).items.push(item);
    }

    const container = $("wardrobe-groups");
    container.innerHTML = "";
    const ordered = [...groups.keys()].sort(
        (a, b) => CATEGORY_ORDER.indexOf(a) - CATEGORY_ORDER.indexOf(b));
    for (const slug of ordered) {
        const group = groups.get(slug);
        const title = document.createElement("h3");
        title.className = "group-title";
        title.textContent =
            `${CATEGORY_EMOJI[slug] || "👔"} ${group.name} (${group.items.length})`;
        container.appendChild(title);

        const grid = document.createElement("ul");
        grid.className = "wardrobe-grid";
        for (const item of group.items) {
            grid.appendChild(wardrobeCard(item));
        }
        container.appendChild(grid);
    }
}

function wardrobeCard(item) {
    const li = document.createElement("li");

    const visual = document.createElement("div");
    visual.className = "card-visual";
    if (item.photo_url) {
        const img = document.createElement("img");
        img.src = item.photo_url;
        img.alt = item.label || item.type_name;
        img.loading = "lazy";
        visual.appendChild(img);
    } else {
        visual.classList.add("emoji");
        visual.textContent = CATEGORY_EMOJI[item.category_slug] || "👔";
    }

    const name = document.createElement("span");
    name.className = "card-name";
    name.textContent = item.label || item.type_name;
    const details = document.createElement("span");
    details.className = "category";
    const parts = item.values.map((v) => v.name);
    if (item.label) parts.unshift(item.type_name);
    details.textContent = parts.join(" · ");

    const del = document.createElement("button");
    del.className = "delete-btn";
    del.textContent = "✕";
    del.title = t("delete");
    del.addEventListener("click", async () => {
        try {
            await fetchJson(`/api/wardrobe/${item.id}`, { method: "DELETE" });
            await loadWardrobe();
        } catch (err) {
            console.error(err);
            showError(t("error"), "wardrobe-error");
        }
    });

    li.append(visual, name, details, del);
    return li;
}

// Bulk add: classify and save each selected photo, reporting per-file
// progress. Files are processed one by one to keep the API load gentle.
async function bulkUpload() {
    const files = [...$("bulk-input").files];
    if (files.length === 0) return;
    $("bulk-input").value = "";
    const progress = $("bulk-progress");
    progress.innerHTML = "";

    for (const file of files) {
        const li = document.createElement("li");
        const label = document.createElement("span");
        label.textContent = file.name;
        const status = document.createElement("span");
        status.className = "bulk-status";
        status.textContent = t("bulk_reading");
        li.append(label, status);
        progress.appendChild(li);

        try {
            const res = await fetch("/api/classify-photo", {
                method: "POST",
                headers: { "Content-Type": file.type },
                body: file,
            });
            const classified = await res.json();
            if (!res.ok) {
                status.textContent =
                    classified.code === "no_api_key" ? t("ask_no_key") : t("bulk_failed");
                li.classList.add("failed");
                continue;
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
        } catch (err) {
            console.error(err);
            status.textContent = t("bulk_failed");
            li.classList.add("failed");
        }
    }
    await loadWardrobe();
}

async function classifyPhoto() {
    const file = $("photo-input").files[0];
    const note = $("classify-note");
    if (!file) {
        note.classList.add("hidden");
        return;
    }
    note.textContent = t("classifying");
    note.classList.remove("hidden");
    try {
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

        const file = $("photo-input").files[0];
        if (file) {
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
        if (!$("result").classList.contains("hidden")) await recommend();
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

$("recommend-btn").addEventListener("click", recommend);
$("ask-btn").addEventListener("click", ask);
$("ask-input").addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
        e.preventDefault();
        ask();
    }
});
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
});
$("gender-select").value = localStorage.getItem("gender") || "";
$("gender-select").addEventListener("change", () => {
    localStorage.setItem("gender", $("gender-select").value);
});

initHourSelects();
refreshAll().catch((err) => {
    console.error(err);
    showError(STRINGS[lang].error);
});
loadLocation();
