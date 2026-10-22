// MUSICAS DO TITULO. GET /musica/v1/<imdb>?t=<titulo>&y=<ano>[&s=<temporada>&country=us]
// Apple Music catalog first when signing secrets exist; MusicBrainz/iTunes fallback.
// Sem sessao: so o id publico e o titulo. Trilha oficial = release-group
// "soundtrack" do MusicBrainz (CC0, 1 req/s, User-Agent obrigatorio); capa e
// link e previas do album vem do iTunes Search. Resultado (inclusive "nao achei") fica em
// KV: 30 dias se achou, 7 se nao. Trilha errada e pior que nenhuma: so aceita
// identidade do titulo e ano a +-1 (temporadas preservam o numero).
// /m/ serve a pagina do celular (QR da TV): links Spotify, Apple e YouTube Music.

const ID_IMDB = /^tt\d{5,10}$/;
const UA = "NuvioLegacy/2.1 (https://github.com/nuvio-legacy)";
const MB = "https://musicbrainz.org/ws/2/";
export const CACHE_ACHOU = 30 * 86400, CACHE_VAZIO = 7 * 86400;
const FAIXAS_MAX = 60;

const resp = (o, status = 200, extra = {}) => new Response(JSON.stringify(o), {
  status, headers: { "content-type": "application/json; charset=utf-8", "access-control-allow-origin": "*", ...extra },
});

export const norm = (s) => String(s || "").normalize("NFD").replace(/[\u0300-\u036f]/g, "")
  .toLowerCase().replace(/[^a-z0-9]+/g, " ").trim();

const json = async (buscar, url) => {
  const r = await buscar(url, { headers: { accept: "application/json", "user-agent": UA } });
  if (!r.ok) throw new Error("http " + r.status);
  return r.json();
};

// Provider search scores rank results; identity comes from title, type and year.
const nomeAlbum = (s) => {
  const n = norm(s).replace(/\b(?:temporada|series) (\d+)\b/g, "season $1");
  const season = n.match(/\bseason \d+\b/)?.[0];
  const title = n.replace(/\bseason \d+\b/g, "").replace(/\b(?:original (?:motion picture |television |tv |game )?(?:soundtrack|score)|music (?:from|of) (?:the |a )?(?:motion picture|film|television|tv|hbo series)|soundtrack|score|ost)\b.*$/, "").trim();
  return `${title}${season ? ` ${season}` : ""}`;
};
const nomeArtista = (s) => String(s || "").split(/\s*&\s*|\s+and\s+/i).map(norm).sort().join(" & ");
const https = (s) => typeof s === "string" && s.startsWith("https://") ? s : "";

// Exact soundtrack title (including season), year +-1 for films.
export function escolher(grupos, titulo, ano, temp) {
  const t = norm(titulo);
  let melhor = null;
  for (const g of grupos || []) {
    const n = norm(g.title);
    if (nomeAlbum(g.title) !== (temp ? `${t} season ${temp}` : t)) continue;
    if (!(g["secondary-types"] || []).some((type) => norm(type) === "soundtrack") &&
        !/\b(soundtrack|score|ost|music from|music of)\b/.test(n)) continue;
    const a = parseInt(String(g["first-release-date"] || "").slice(0, 4), 10);
    if (ano && !(Number.isFinite(a) && Math.abs(a - ano) <= (temp ? 15 : 1))) continue;
    const eTemp = temp && new RegExp("(season|temporada|series) " + temp + "\\b").test(n);
    if (temp && !eTemp) continue;
    if (!melhor || (g.score ?? 0) > (melhor.score ?? 0)) melhor = g;
  }
  return melhor;
}

const artista = (ac) => (ac || []).map((c) => (c.name || "") + (c.joinphrase || "")).join("").trim();

async function procurar(buscar, titulo, ano, temp) {
  const termo = temp ? `${titulo} season ${temp}` : titulo;
  const q = `releasegroup:"${termo.replace(/["\\]/g, " ")}" AND (secondarytype:soundtrack OR releasegroup:score OR releasegroup:soundtrack)`;
  const d = await json(buscar, `${MB}release-group?query=${encodeURIComponent(q)}&limit=8&fmt=json`);
  return escolher(d["release-groups"], titulo, ano, temp);
}

async function capaApple(buscar, album, art, ano, faixas, country) {
  try {
    const q = encodeURIComponent((nomeAlbum(album) + " " + art).slice(0, 120));
    const d = await json(buscar, `https://itunes.apple.com/search?term=${q}&entity=album&limit=8&country=${country}`);
    const a = (d.results || []).find((x) => nomeAlbum(x.collectionName) === nomeAlbum(album) &&
      nomeArtista(x.artistName) === nomeArtista(art) &&
      Math.abs(parseInt(x.releaseDate, 10) - ano) <= 1);
    if (!a) return {};
    const ap = { capa: https(a.artworkUrl100).replace(/\d+x\d+bb/, "600x600bb"), apple: https(a.collectionViewUrl), appleUrl: https(a.collectionViewUrl) };
    try {
      const songs = await json(buscar, `https://itunes.apple.com/lookup?id=${encodeURIComponent(a.collectionId)}&entity=song&limit=200&country=${country}`);
      for (const k of faixas) {
        const matches = (songs.results || []).filter((x) => x.kind === "song" && norm(x.trackName) === norm(k.titulo) &&
          nomeArtista(x.artistName) === nomeArtista(k.artista || art) &&
          (!k.dur || Math.abs(x.trackTimeMillis / 1000 - k.dur) <= 3));
        if (matches.length === 1 && https(matches[0].previewUrl)) k.previa = matches[0].previewUrl;
      }
    } catch { /* Artwork remains useful when the optional song lookup fails. */ }
    return ap;
  } catch { return {}; }
}

let developerToken;
const appleConfigured = (env) => env?.APPLE_MUSIC_TEAM_ID && env?.APPLE_MUSIC_KEY_ID && env?.APPLE_MUSIC_P8;
const base64url = (bytes) => btoa(String.fromCharCode(...bytes)).replace(/=/g, "").replace(/\+/g, "-").replace(/\//g, "_");

async function tokenApple(env, agora) {
  const { APPLE_MUSIC_TEAM_ID: team, APPLE_MUSIC_KEY_ID: kid, APPLE_MUSIC_P8: pem } = env;
  if (developerToken?.team === team && developerToken.kid === kid && developerToken.pem === pem && developerToken.exp > agora + 60) return developerToken.token;
  const bytes = Uint8Array.from(atob(pem.replace(/-----[^-]+-----|\s/g, "")), (c) => c.charCodeAt(0));
  const key = await crypto.subtle.importKey("pkcs8", bytes, { name: "ECDSA", namedCurve: "P-256" }, false, ["sign"]);
  const encode = (value) => base64url(new TextEncoder().encode(JSON.stringify(value)));
  const exp = agora + 12 * 3600;
  const body = `${encode({ alg: "ES256", kid })}.${encode({ iss: team, iat: agora, exp })}`;
  const signature = await crypto.subtle.sign({ name: "ECDSA", hash: "SHA-256" }, key, new TextEncoder().encode(body));
  const token = `${body}.${base64url(new Uint8Array(signature))}`;
  developerToken = { team, kid, pem, exp, token };
  return token;
}

async function catalogoApple(buscar, env, titulo, ano, temp, country, agora) {
  if (!appleConfigured(env)) return null;
  try {
    const token = await tokenApple(env, Math.floor(agora / 1000));
    const get = async (path) => {
      const r = await buscar(`https://api.music.apple.com/v1/catalog/${country}/${path}`, {
        headers: { accept: "application/json", authorization: `Bearer ${token}` },
      });
      if (!r.ok) throw new Error("Apple catalog unavailable");
      return r.json();
    };
    const term = `${titulo}${temp ? ` season ${temp}` : ""} soundtrack`;
    const search = await get(`search?types=albums&limit=25&term=${encodeURIComponent(term)}`);
    const albums = search.results?.albums?.data || [];
    const match = escolher(albums.map(({ id, attributes: a }) => ({
      id, title: a?.name, "first-release-date": a?.releaseDate,
      "secondary-types": a?.genreNames?.some((g) => /soundtrack|score/i.test(g)) ? ["Soundtrack"] : [],
    })), titulo, ano, temp);
    if (!match) return null;
    const result = await get(`albums/${encodeURIComponent(match.id)}?include=tracks`);
    const album = result.data?.[0], a = album?.attributes;
    if (!a) return null;
    const faixas = (album.relationships?.tracks?.data || []).filter((k) => k.type === "songs" && k.attributes?.name)
      .slice(0, FAIXAS_MAX).map(({ attributes: k }, i) => ({
        n: i + 1, titulo: k.name, artista: k.artistName || a.artistName,
        dur: Math.round((k.durationInMillis || 0) / 1000),
        ...(https(k.previews?.[0]?.url) ? { previa: k.previews[0].url } : {}),
      }));
    if (!faixas.length) return null;
    const cores = Object.fromEntries(["bgColor", "textColor1", "textColor2", "textColor3", "textColor4"]
      .filter((key) => /^[a-f0-9]{6}$/i.test(a.artwork?.[key] || "")).map((key) => [key, a.artwork[key]]));
    return { album: { titulo: a.name, artista: a.artistName, ano: parseInt(a.releaseDate, 10),
      genero: (a.genreNames || []).join(", "), nota: a.editorialNotes?.short || "",
      capa: https(a.artwork?.url).replace(/\{w\}|\{h\}/g, "600"), cores,
      apple: https(a.url), appleUrl: https(a.url) }, faixas };
  } catch { return null; } // Optional provider: never log signing material or tokens.
}

export async function trilha(buscar, titulo, ano, temp, country = "us") {
  const g = await procurar(buscar, titulo, ano, temp);
  if (!g) return null;
  const rel = await json(buscar, `${MB}release?release-group=${g.id}&inc=recordings+artist-credits&limit=1&fmt=json`);
  const r = rel.releases?.[0];
  const faixas = (r?.media || []).flatMap((m) => m.tracks || []).slice(0, FAIXAS_MAX).map((k, i) => ({
    n: i + 1, titulo: String(k.title || ""), artista: artista(k["artist-credit"]),
    dur: Math.round((k.length || 0) / 1000),
  })).filter((k) => k.titulo);
  if (!faixas.length) return null;
  const art = artista(g["artist-credit"]);
  const ap = await capaApple(buscar, g.title, art, parseInt(g["first-release-date"], 10), faixas, country);
  return { album: { titulo: g.title, artista: art, ano: parseInt(g["first-release-date"], 10), genero: "Soundtrack", ...ap }, faixas };
}

export async function rotaMusica(url, env, buscar = fetch, limite = null, ip = "0.0.0.0", agora = Date.now()) {
  const m = /^\/musica\/v1\/(tt\d+)$/.exec(url.pathname);
  const titulo = (url.searchParams.get("t") || "").slice(0, 120).trim();
  const ano = parseInt(url.searchParams.get("y") || "", 10) || 0;
  const temp = parseInt(url.searchParams.get("s") || "", 10) || 0;
  if (!m || !ID_IMDB.test(m[1]) || !titulo || /[\u0000-\u001f]/.test(titulo)) return resp({ erro: "pedido invalido" }, 400);
  const requestedCountry = (url.searchParams.get("country") || "us").toLowerCase();
  const country = /^[a-z]{2}$/.test(requestedCountry) ? requestedCountry : "us";
  const chave = `m4:${m[1]}:${temp}:${country}:${appleConfigured(env) ? "apple" : "mb"}`;
  const guardado = await env?.MUS?.get(chave);
  if (guardado) return resp(JSON.parse(guardado), 200, { "cache-control": "public, max-age=3600" });
  if (limite && !(await limite(env, "musica:" + ip, 30, 600, Math.floor(agora / 1000)))) return resp({ erro: "muitos pedidos" }, 429);
  let t;
  try {
    const tipo = temp > 0 ? "series" : "movie";
    const d = await json(buscar, `https://v3-cinemeta.strem.io/meta/${tipo}/${m[1]}.json`);
    const nome = String(d.meta?.name || "").trim();
    const anoId = parseInt(d.meta?.year || d.meta?.releaseInfo || "", 10);
    if (!nome || !Number.isFinite(anoId)) throw new Error("missing canonical metadata");
    t = await catalogoApple(buscar, env, nome, anoId, temp, country, agora) ||
      await trilha(buscar, nome, anoId, temp, country);
  } catch { return resp({ erro: "fonte indisponivel" }, 502); }
  const corpo = t || { vazio: true };
  await env?.MUS?.put(chave, JSON.stringify(corpo), { expirationTtl: t ? CACHE_ACHOU : CACHE_VAZIO });
  return resp(corpo, 200, { "cache-control": "public, max-age=3600" });
}

// Pagina do celular. O QR leva /m/#id=tt..&t=..&y=..&s=..; a pagina pergunta
// ao proprio worker e so monta links (open.spotify.com/search nao usa API).
export const PAGINA = `<!doctype html><html lang="pt"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Nuvio</title>
<style>body{margin:0;font:16px system-ui;background:#10131a;color:#eef}main{max-width:560px;margin:auto;padding:16px}h1{font-size:20px}li{list-style:none;padding:10px 0;border-bottom:1px solid #fff2}b{display:block}small{opacity:.7}a{color:#8cf;margin-right:14px;text-decoration:none;font-size:15px}img{width:120px;border-radius:12px}</style></head>
<body><main><h1 id="h">...</h1><div id="c"></div><ul id="l" style="padding:0"></ul></main>
<script>
var p=new URLSearchParams(location.hash.slice(1)),q=encodeURIComponent;
fetch("/musica/v1/"+q(p.get("id")||"x")+"?t="+q(p.get("t")||"")+"&y="+q(p.get("y")||"")+"&s="+q(p.get("s")||"")).then(function(r){return r.json()}).then(function(d){
var h=document.getElementById("h"),l=document.getElementById("l");
if(!d.faixas){h.textContent=p.get("t")||"";return}
h.textContent=d.album.titulo;
if(d.album.capa){var i=new Image();i.src=d.album.capa;document.getElementById("c").appendChild(i)}
d.faixas.forEach(function(k){var s=q(k.titulo+" "+k.artista),li=document.createElement("li"),b=document.createElement("b");
b.textContent=k.titulo;var sm=document.createElement("small");sm.textContent=k.artista;
li.appendChild(b);li.appendChild(sm);
[["Spotify","https://open.spotify.com/search/"+s],["Apple Music","https://music.apple.com/search?term="+s],["YouTube Music","https://music.youtube.com/search?q="+s]].forEach(function(x){var a=document.createElement("a");a.href=x[1];a.textContent=x[0];li.appendChild(a)});
l.appendChild(li)})}).catch(function(){document.getElementById("h").textContent="?"});
</script></body></html>`;

export const rotaPaginaMusica = () => new Response(PAGINA, {
  headers: { "content-type": "text/html; charset=utf-8", "cache-control": "public, max-age=3600", "referrer-policy": "no-referrer" },
});
