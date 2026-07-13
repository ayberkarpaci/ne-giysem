const STRINGS = {
    en: {
        mood_title: "How do you feel today?",
        recommend: "What should I wear?",
        loading: "Checking the weather...",
        weather: (t, rainy, city) =>
            `${t} °C, ${rainy ? "rainy" : "dry"}${city ? " in " + city : ""}`,
        error: "Could not get a recommendation. Is the network up?",
        footer: "Weather by open-meteo.com · location by ip-api.com",
    },
    tr: {
        mood_title: "Bugün nasıl hissediyorsun?",
        recommend: "Ne giysem?",
        loading: "Hava durumuna bakılıyor...",
        weather: (t, rainy, city) =>
            `${t} °C, ${rainy ? "yağmurlu" : "kuru"}${city ? ", " + city : ""}`,
        error: "Öneri alınamadı. Ağ bağlantısını kontrol et.",
        footer: "Hava durumu: open-meteo.com · konum: ip-api.com",
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

const $ = (id) => document.getElementById(id);

function applyStaticStrings() {
    document.documentElement.lang = lang;
    document.querySelectorAll("[data-i18n]").forEach((el) => {
        const value = STRINGS[lang][el.dataset.i18n];
        if (typeof value === "string") el.textContent = value;
    });
    document.querySelectorAll(".lang-switch button").forEach((btn) => {
        btn.classList.toggle("active", btn.dataset.lang === lang);
    });
}

async function loadMoods() {
    const res = await fetch(`/api/moods?lang=${lang}`);
    const data = await res.json();
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
    btn.textContent = STRINGS[lang].loading;
    $("error").classList.add("hidden");
    try {
        const res = await fetch(`/api/recommendation?mood=${mood}&lang=${lang}`);
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const data = await res.json();

        $("weather-line").textContent = STRINGS[lang].weather(
            Math.round(data.weather.temperature_c * 10) / 10,
            data.weather.is_raining,
            data.weather.city,
        );

        const list = $("outfit-list");
        list.innerHTML = "";
        for (const item of data.outfit) {
            const li = document.createElement("li");
            const emoji = CATEGORY_EMOJI[item.category_slug] || "👔";
            li.innerHTML =
                `<span class="emoji"></span>` +
                `<span><span class="category"></span><span class="name"></span></span>`;
            li.querySelector(".emoji").textContent = emoji;
            li.querySelector(".category").textContent = item.category_name;
            li.querySelector(".name").textContent = item.item_name;
            list.appendChild(li);
        }
        $("result").classList.remove("hidden");
    } catch (err) {
        console.error(err);
        $("error").textContent = STRINGS[lang].error;
        $("error").classList.remove("hidden");
    } finally {
        btn.disabled = false;
        btn.textContent = STRINGS[lang].recommend;
    }
}

document.querySelectorAll(".lang-switch button").forEach((btn) => {
    btn.addEventListener("click", async () => {
        lang = btn.dataset.lang;
        localStorage.setItem("lang", lang);
        applyStaticStrings();
        await loadMoods();
        if (!$("result").classList.contains("hidden")) await recommend();
    });
});

$("recommend-btn").addEventListener("click", recommend);

applyStaticStrings();
loadMoods().catch((err) => {
    console.error(err);
    $("error").textContent = STRINGS[lang].error;
    $("error").classList.remove("hidden");
});
