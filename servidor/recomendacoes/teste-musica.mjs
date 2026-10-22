// Teste de /musica/v1 e /m/ (src/musica.js) sem rede: MusicBrainz e iTunes sao dobles.
//   node servidor/recomendacoes/teste-musica.mjs
import assert from "node:assert/strict";
import { webcrypto } from "node:crypto";
globalThis.crypto ||= webcrypto;
import { rotaMusica, rotaPaginaMusica, escolher, CACHE_ACHOU, CACHE_VAZIO } from "./src/musica.js";

const ok = (nome, cond) => { assert.ok(cond, nome); console.log("ok", nome); };
const U = (s) => new URL("https://rec.exemplo.dev" + s);
const pedidos = [];
const J = (o) => new Response(JSON.stringify(o), { status: 200 });
const fonte = (grupos) => async (alvo, opts) => {
  pedidos.push({ alvo, opts });
  if (alvo.includes("v3-cinemeta.strem.io/meta/")) return J({ meta: { name: "Inception", year: "2010" } });
  if (alvo.includes("/release-group?")) return J({ "release-groups": grupos });
  if (alvo.includes("/release?")) return J({ releases: [{ media: [{ tracks: [
    { title: "Time", length: 275000, "artist-credit": [{ name: "Hans Zimmer" }] },
    { title: "Mind Heist", length: 192000, "artist-credit": [{ name: "Hans Zimmer" }] }] }] }] });
  if (alvo.includes("itunes.apple.com")) return J({ results: [{ collectionId: 1, artistName: "Hans Zimmer", releaseDate: "2010-07-13", collectionName: "Inception (Music from the Motion Picture)",
    artworkUrl100: "https://is1.mzstatic.com/a/100x100bb.jpg", collectionViewUrl: "https://music.apple.com/us/album/x" }] });
  throw new Error("alvo inesperado " + alvo);
};
const OST = { id: "g1", title: "Inception (Music from the Motion Picture)", score: 100, "first-release-date": "2010-07-13", "artist-credit": [{ name: "Hans Zimmer" }] };
const kv = () => { const m = new Map(), ttl = []; return { m, ttl, get: async (k) => m.get(k) ?? null, put: async (k, v, o) => { m.set(k, v); ttl.push(o.expirationTtl); } }; };

// Interstellar regression: provider rank is not confidence for an exact soundtrack.
const INTERSTELLAR = { ...OST, id: "interstellar", title: "Interstellar: Original Motion Picture Soundtrack",
  score: 83, "secondary-types": ["Soundtrack"], "first-release-date": "2014-11-17" };
const preview = "https://audio-ssl.itunes.apple.com/Time.m4a";
const interstellarSource = (mode = "ok") => async (target) => {
  const u = new URL(target);
  if (u.hostname === "v3-cinemeta.strem.io") return J({ meta: { name: "Interstellar", year: "2014" } });
  if (u.pathname.endsWith("/release-group")) {
    assert.match(u.searchParams.get("query"), /soundtrack/);
    return J({ "release-groups": [INTERSTELLAR] });
  }
  if (u.pathname.endsWith("/release")) return J({ releases: [{ media: [{ tracks: [
    { title: "Dreaming of the Crash", length: 235000, "artist-credit": [{ name: "Hans Zimmer" }] },
    { title: "Cornfield Chase", length: 126000, "artist-credit": [{ name: "Hans Zimmer" }] },
    { title: "Dust", length: 341000, "artist-credit": [{ name: "Hans Zimmer" }] },
    { title: "Day One", length: 199000, "artist-credit": [{ name: "Hans Zimmer" }] },
    { title: "Stay", length: 412000, "artist-credit": [{ name: "Hans Zimmer" }] },
  ] }] }] });
  if (mode === "offline") throw new Error("Apple unavailable");
  if (u.pathname === "/search") return J({ results: [
    { collectionId: 9, collectionName: "Interstellar (Original Motion Picture Soundtrack)", artistName: "Tribute Orchestra", releaseDate: "2014-01-01" },
    { collectionId: 10, collectionName: "Interstellar (Original Motion Picture Soundtrack)", artistName: "Hans Zimmer", releaseDate: "2014-11-17",
      artworkUrl100: "https://is1.mzstatic.com/art/100x100bb.jpg", collectionViewUrl: "https://music.apple.com/us/album/10" },
  ] });
  if (u.pathname === "/lookup") {
    assert.equal(u.searchParams.get("id"), "10", "wrong artist album must be skipped");
    return J({ results: [
      { wrapperType: "collection" },
      { kind: "song", trackName: "Cornfield Chase", artistName: "Hans Zimmer", trackTimeMillis: 126000, previewUrl: preview },
      { kind: "song", trackName: "Dreaming of the Crash", artistName: "Hans Zimmer", trackTimeMillis: 235000, previewUrl: "https://audio-ssl.itunes.apple.com/Crash.m4a" },
      { kind: "song", trackName: "Dust", artistName: "Hans Zimmer", trackTimeMillis: 341000, previewUrl: "javascript:bad" },
      { kind: "song", trackName: "Day One", artistName: "Tribute Orchestra", trackTimeMillis: 199000, previewUrl: preview },
      { kind: "song", trackName: "Stay", artistName: "Hans Zimmer", trackTimeMillis: 5000, previewUrl: preview },
    ] });
  }
  throw new Error("Unexpected fixture request " + target);
};
const interEnv = { MUS: kv() };
interEnv.MUS.m.set("m2:tt0816692:0", JSON.stringify({ vazio: true }));
const interResponse = await rotaMusica(U("/musica/v1/tt0816692?t=Interstellar&y=2014"), interEnv, interstellarSource());
const inter = await interResponse.json();
assert.equal(interResponse.status, 200);
assert.equal(inter.album?.capa, "https://is1.mzstatic.com/art/600x600bb.jpg", "Interstellar enriches despite title variant and old empty cache");
assert.equal(inter.faixas[1].previa, preview, "preview follows track identity, not lookup order");
assert.match(inter.faixas[0].previa, /Crash\.m4a$/);
assert.ok(inter.faixas.slice(2).every((track) => !track.previa), "unsafe, wrong-artist and wrong-duration previews are omitted");
const unavailable = await rotaMusica(U("/musica/v1/tt0816692?t=Interstellar&y=2014"), { MUS: kv() }, interstellarSource("offline"));
assert.equal((await unavailable.json()).faixas.length, 5, "Apple failure preserves MusicBrainz tracks");
for (const year of [2013, 2015]) assert.ok(escolher([{ ...INTERSTELLAR, "first-release-date": String(year) }], "Interstellar", 2014, 0));
for (const group of [
  { ...INTERSTELLAR, "first-release-date": "2012" },
  { ...INTERSTELLAR, title: "Interstellar Tribute" },
  { ...INTERSTELLAR, title: "Not Interstellar" },
]) assert.equal(escolher([group], "Interstellar", 2014, 0), null);
ok("Interstellar soundtrack, artwork, safe per-track previews and optional Apple outage", true);

let env = { MUS: kv() };
let r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), env, fonte([OST]));
let d = await r.json();
ok("achou: faixas com titulo, artista e duracao", r.status === 200 && d.faixas.length === 2 && d.faixas[0].dur === 275 && d.faixas[1].artista === "Hans Zimmer");
ok("achou: capa 600 https e link da Apple", d.album.capa.includes("600x600bb") && d.album.apple.startsWith("https://music.apple.com/"));
ok("MusicBrainz leva User-Agent", pedidos.find((p) => p.alvo.includes("musicbrainz.org")).opts.headers["user-agent"].startsWith("NuvioLegacy"));
ok("cache 30 dias", env.MUS.ttl[0] === CACHE_ACHOU);
const n = pedidos.length;
await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), env, fonte([OST]));
ok("segunda consulta sai do KV", pedidos.length === n);

env = { MUS: kv() };
r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2014"), env, fonte([OST]));
ok("caller year cannot poison cache", (await r.json()).faixas.length === 2 && env.MUS.ttl[0] === CACHE_ACHOU);

env = { MUS: kv() };
r = await rotaMusica(U("/musica/v1/tt1375666?t=Unrelated&y=1999"), env, fonte([OST]));
assert.equal((await r.json()).album?.titulo, OST.title, "caller title cannot poison cache");
r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), env, fonte([OST]));
assert.equal((await r.json()).album?.titulo, OST.title, "legitimate lookup survives hostile first request");
env = { MUS: kv() };
r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), env, fonte([]));
ok("canonical miss cached seven days", (await r.json()).vazio && env.MUS.ttl[0] === CACHE_VAZIO);
ok("exact soundtrack identity survives low provider rank", escolher([{ ...OST, score: 80 }], "Inception", 2010, 0) !== null);
ok("escolher: titulo diferente recusado", escolher([{ ...OST, title: "Interstellar OST" }], "Inception", 2010, 0) === null);
ok("escolher: temporada certa", escolher([{ ...OST, title: "Show: Season 2 (Score)", "first-release-date": "2020-01-01" }], "Show", 2019, 2) !== null
  && escolher([{ ...OST, title: "Show: Season 1", "first-release-date": "2019-01-01" }], "Show", 2019, 2) === null);

for (const ruim of ["/musica/v1/xx1?t=a&y=1", "/musica/v1/tt1375666", "/musica/v1/tt1375666?t=%00x", "/musica/v1/tt1/../x?t=a"]) {
  pedidos.length = 0;
  r = await rotaMusica(U(ruim), { MUS: kv() }, fonte([OST]));
  assert.equal(r.status, 400, ruim);
  assert.equal(pedidos.length, 0, ruim);
}
ok("pedido invalido: 400 sem pedir nada", true);

r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), { MUS: kv() }, async () => { throw new Error("x"); });
ok("fonte caida: 502 e nada no cache", r.status === 502);
r = await rotaMusica(U("/musica/v1/tt1375666?t=Inception&y=2010"), { MUS: kv() }, fonte([OST]), async () => false);
ok("limite estourado: 429", r.status === 429);

const reversedArtists = async (target, options) => {
  const response = await interstellarSource()(target, options);
  const data = await response.json();
  if (new URL(target).pathname === "/search") data.results[1].artistName = "James Newton Howard & Hans Zimmer";
  if (new URL(target).pathname.endsWith("/release-group")) data["release-groups"] = data["release-groups"].map((group) => ({ ...group,
    "artist-credit": [{ name: "Hans Zimmer", joinphrase: " & " }, { name: "James Newton Howard" }] }));
  return J(data);
};
assert.ok((await (await rotaMusica(U("/musica/v1/tt0816692?t=Interstellar&y=2014"), { MUS: kv() }, reversedArtists)).json()).album.capa,
  "co-composer order does not discard album artwork");
assert.ok(escolher([{ ...OST, title: "Show (Music from the HBO Series, Season 2)", "first-release-date": "2020" }], "Show", 2019, 2));
assert.equal(escolher([{ ...OST, title: "Show (Music from the HBO Series, Season 1)", "first-release-date": "2020" }], "Show", 2019, 2), null);

// Real ES256 signatures with generated test-only keys; every HTTP request is a fixture.
const keys = await crypto.subtle.generateKey({ name: "ECDSA", namedCurve: "P-256" }, true, ["sign", "verify"]);
const p8 = Buffer.from(await crypto.subtle.exportKey("pkcs8", keys.privateKey)).toString("base64");
const appleEnv = { MUS: kv(), APPLE_MUSIC_TEAM_ID: "fixture-team", APPLE_MUSIC_KEY_ID: "fixture-key",
  APPLE_MUSIC_P8: `-----BEGIN PRIVATE KEY-----\n${p8}\n-----END PRIVATE KEY-----` };
const tokens = [], stores = [];
const clock = 1800000000000;
const catalog = (mode = "ok") => async (target, options) => {
  const u = new URL(target);
  if (u.hostname !== "api.music.apple.com") return interstellarSource()(target, options);
  if (mode === "outage") return new Response("unavailable", { status: 503 });
  const token = options.headers.authorization.replace("Bearer ", "");
  tokens.push(token);
  stores.push(u.pathname.split("/")[3]);
  const [header, payload, signature] = token.split(".");
  assert.deepEqual(JSON.parse(Buffer.from(header, "base64url")), { alg: "ES256", kid: "fixture-key" });
  const claims = JSON.parse(Buffer.from(payload, "base64url"));
  assert.equal(claims.iss, "fixture-team");
  assert.equal(claims.exp - claims.iat, 12 * 3600);
  assert.ok(await crypto.subtle.verify({ name: "ECDSA", hash: "SHA-256" }, keys.publicKey,
    Buffer.from(signature, "base64url"), new TextEncoder().encode(`${header}.${payload}`)));
  const attributes = { name: "Interstellar (Original Motion Picture Soundtrack)", artistName: "Hans Zimmer",
    releaseDate: "2014-11-17", genreNames: ["Soundtrack"], url: "https://music.apple.com/br/album/10",
    artwork: { url: "https://is1.mzstatic.com/art/{w}x{h}bb.jpg", bgColor: "152c38", textColor1: "ffffff", textColor2: "eeeeee", textColor3: "aaaaaa", textColor4: "999999" },
    editorialNotes: { short: "A journey beyond the stars." } };
  if (u.pathname.endsWith("/search")) {
    assert.match(u.searchParams.get("term"), /Interstellar soundtrack/);
    assert.equal(u.searchParams.get("types"), "albums");
    return J({ results: { albums: { data: [
      { id: "wrong", attributes: { ...attributes, name: "Not Interstellar (Soundtrack)" } },
      { id: "old", attributes: { ...attributes, releaseDate: "1999-01-01" } },
      { id: "10", attributes },
    ] } } });
  }
  assert.match(u.pathname, /\/albums\/10$/);
  return J({ data: [{ attributes, relationships: { tracks: { data: mode === "empty" ? [] : [
    { type: "songs", attributes: { name: "Cornfield Chase", artistName: "Hans Zimmer", durationInMillis: 126000, previews: [{ url: preview }] } },
    { type: "songs", attributes: { name: "Dust", artistName: "Hans Zimmer", durationInMillis: 341000, previews: [{ url: "http://unsafe.test/a.m4a" }] } },
  ] } } }] });
};
const appleRequest = (country = "br") => U(`/musica/v1/tt0816692?t=Interstellar&y=2014&country=${country}`);
const appleResponse = await rotaMusica(appleRequest(), appleEnv, catalog(), null, "fixture", clock);
const apple = await appleResponse.json();
assert.equal(apple.album.capa, "https://is1.mzstatic.com/art/600x600bb.jpg");
assert.equal(apple.album.cores.bgColor, "152c38");
assert.equal(apple.album.cores.textColor4, "999999");
assert.equal(apple.album.ano, 2014);
assert.equal(apple.album.genero, "Soundtrack");
assert.equal(apple.album.nota, "A journey beyond the stars.");
assert.equal(apple.album.appleUrl, apple.album.apple);
assert.equal(apple.faixas[0].previa, preview);
assert.equal(apple.faixas[0].dur, 126);
assert.ok(!apple.faixas[1].previa);
assert.ok(!(JSON.stringify(apple).includes(tokens[0])), "developer token never enters response");
await rotaMusica(appleRequest("us"), appleEnv, catalog(), null, "fixture", clock + 1000);
assert.deepEqual(stores, ["br", "br", "us", "us"]);
assert.equal(tokens[0], tokens[2], "developer token reused across storefronts");
appleEnv.MUS = kv();
await rotaMusica(appleRequest("invalid"), appleEnv, catalog(), null, "fixture", clock + 13 * 3600000);
assert.equal(stores.at(-1), "us", "invalid country defaults to us");
assert.notEqual(tokens.at(-1), tokens[0], "developer token renewed after twelve hours");
for (const mode of ["outage", "empty"]) {
  const fallback = await rotaMusica(appleRequest(), { ...appleEnv, MUS: kv() }, catalog(mode));
  assert.equal((await fallback.json()).faixas.length, 5, `${mode} falls back to MusicBrainz/iTunes`);
}
const badKey = await rotaMusica(appleRequest(), { ...appleEnv, MUS: kv(), APPLE_MUSIC_P8: "invalid" }, interstellarSource());
assert.equal((await badKey.json()).faixas.length, 5, "invalid signing key falls back");
ok("Apple catalog first, ES256, token renewal, storefront isolation and fallback", true);

r = rotaPaginaMusica();
const html = await r.text();
ok("pagina: html, sem referrer, links Spotify/Apple/YouTube", r.headers.get("content-type").startsWith("text/html") &&
  r.headers.get("referrer-policy") === "no-referrer" && ["open.spotify.com/search", "music.apple.com/search", "music.youtube.com/search"].every((x) => html.includes(x)));
