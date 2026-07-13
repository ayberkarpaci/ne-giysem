const STRINGS = {
    en: {
        mood_title: "How do you feel today?",
        recommend: "What should I wear?",
        loading: "Checking the weather...",
        weather: (t, rainy, city) =>
            `${t} °C, ${rainy ? "rainy" : "dry"}${city ? " in " + city : ""}`,
        error: "Could not get a recommendation. Is the network up?",
        save_error: "Could not save the item.",
        footer: "Weather by open-meteo.com · location by ip-api.com",
        use_wardrobe: "Recommend from my wardrobe",
        catalog_fallback: "Your wardrobe is empty, so this comes from the general catalog.",
        ask_title: "Tell me your plan",
        ask_placeholder: "e.g. going to work tomorrow, smart but comfy",
        ask: "Suggest an outfit",
        asking: "Thinking...",
        ask_no_key: "The Gemini API key is not set up yet (see .env).",
        wardrobe_title: "My wardrobe",
        wardrobe_empty: "No items yet — add your first piece below!",
        add_item: "Add clothing item",
        type_label: "Type",
        label_label: "Name (optional)",
        photo_label: "Photo (optional)",
        save: "Save",
        saving: "Saving...",
        delete: "Delete",
        choose: "—",
    },
    tr: {
        mood_title: "Bugün nasıl hissediyorsun?",
        recommend: "Ne giysem?",
        loading: "Hava durumuna bakılıyor...",
        weather: (t, rainy, city) =>
            `${t} °C, ${rainy ? "yağmurlu" : "kuru"}${city ? ", " + city : ""}`,
        error: "Öneri alınamadı. Ağ bağlantısını kontrol et.",
        save_error: "Kıyafet kaydedilemedi.",
        footer: "Hava durumu: open-meteo.com · konum: ip-api.com",
        use_wardrobe: "Gardırobumdan öner",
        catalog_fallback: "Gardırobun boş olduğu için bu öneri genel katalogdan geldi.",
        ask_title: "Planını anlat",
        ask_placeholder: "örn. yarın işe gideceğim, şık ama rahat olsun",
        ask: "Kombin öner",
        asking: "Düşünüyorum...",
        ask_no_key: "Gemini API anahtarı henüz ayarlanmamış (.env dosyasına bak).",
        wardrobe_title: "Gardırobum",
        wardrobe_empty: "Henüz kıyafet yok — aşağıdan ilk parçanı ekle!",
        add_item: "Kıyafet ekle",
        type_label: "Tür",
        label_label: "İsim (isteğe bağlı)",
        photo_label: "Fotoğraf (isteğe bağlı)",
        save: "Kaydet",
        saving: "Kaydediliyor...",
        delete: "Sil",
        choose: "—",
    },
};

const CATEGORY_EMOJI = {
    outerwear: "🧥",
    top: "👕",
    bottom: "👖",
    footwear: "👟",
    accessory: "🧣",
};

let lang = localStorage.getItem("lang") || "en";
let mood = "cozy";
let wardrobeCount = 0;

const $ = (id) => document.getElementById(id);
const t = (key) => STRINGS[lang][key];

function showError(message) {
    $("error").textContent = message;
    $("error").classList.remove("hidden");
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
            mood = m.slug;
            chips.querySelectorAll("button").forEach((b) =>
                b.classList.toggle("selected", b === btn));
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
        const data = await fetchJson(
            `/api/recommendation?mood=${mood}&lang=${lang}&source=${source}`);

        $("weather-line").textContent = t("weather")(
            Math.round(data.weather.temperature_c * 10) / 10,
            data.weather.is_raining,
            data.weather.city,
        );
        $("source-note").classList.toggle("hidden",
            !(useWardrobe && data.source === "catalog"));

        renderOutfitList($("outfit-list"), data.outfit);
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
            body: JSON.stringify({ text, lang }),
        });
        const data = await res.json();
        if (!res.ok) {
            showError(data.code === "no_api_key" ? t("ask_no_key") : t("error"));
            return;
        }
        $("ask-weather-line").textContent = t("weather")(
            Math.round(data.weather.temperature_c * 10) / 10,
            data.weather.is_raining,
            data.weather.city,
        );
        renderOutfitList($("ask-outfit-list"), data.outfit);
        $("ask-explanation").textContent = data.explanation || "";
        $("ask-result").classList.remove("hidden");
    } catch (err) {
        console.error(err);
        showError(t("error"));
    } finally {
        btn.disabled = false;
        btn.textContent = t("ask");
    }
}

async function loadTypes() {
    const data = await fetchJson(`/api/types?lang=${lang}`);
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

async function loadAttributes() {
    const type = $("type-select").value;
    const container = $("attribute-selects");
    container.innerHTML = "";
    if (!type) return;
    const data = await fetchJson(`/api/attributes?type=${type}&lang=${lang}`);
    for (const attr of data.attributes) {
        const label = document.createElement("label");
        const caption = document.createElement("span");
        caption.textContent = attr.name;
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
        container.appendChild(label);
    }
}

async function loadWardrobe() {
    const data = await fetchJson(`/api/wardrobe?lang=${lang}`);
    wardrobeCount = data.items.length;
    $("wardrobe-empty").classList.toggle("hidden", wardrobeCount > 0);
    $("wardrobe-toggle").classList.toggle("hidden", wardrobeCount === 0);

    const list = $("wardrobe-list");
    list.innerHTML = "";
    for (const item of data.items) {
        const li = document.createElement("li");

        const visual = document.createElement("span");
        if (item.photo_url) {
            const img = document.createElement("img");
            img.src = item.photo_url;
            img.alt = item.label || item.type_name;
            img.className = "thumb";
            visual.appendChild(img);
        } else {
            visual.className = "emoji";
            visual.textContent = CATEGORY_EMOJI[item.category_slug] || "👔";
        }

        const text = document.createElement("span");
        text.className = "grow";
        const name = document.createElement("span");
        name.textContent = item.label || item.type_name;
        const details = document.createElement("span");
        details.className = "category";
        const parts = item.values.map((v) => v.name);
        if (item.label) parts.unshift(item.type_name);
        details.textContent = parts.join(" · ");
        text.append(name, details);

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
                showError(t("error"));
            }
        });

        li.append(visual, text, del);
        list.appendChild(li);
    }
}

async function saveItem() {
    const btn = $("save-btn");
    btn.disabled = true;
    btn.textContent = t("saving");
    $("error").classList.add("hidden");
    try {
        const values = [...$("attribute-selects").querySelectorAll("select")]
            .map((s) => s.value)
            .filter((v) => v !== "");
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
        await loadWardrobe();
    } catch (err) {
        console.error(err);
        showError(t("save_error"));
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

$("recommend-btn").addEventListener("click", recommend);
$("ask-btn").addEventListener("click", ask);
$("ask-input").addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
        e.preventDefault();
        ask();
    }
});
$("type-select").addEventListener("change", loadAttributes);
$("save-btn").addEventListener("click", saveItem);

refreshAll().catch((err) => {
    console.error(err);
    showError(STRINGS[lang].error);
});
