#include "lwip/apps/fs.h"

#define file_NULL ((const struct fsdata_file *) NULL)

#ifndef FS_FILE_FLAGS_HEADER_INCLUDED
#define FS_FILE_FLAGS_HEADER_INCLUDED 1
#endif

/* Path strings are padded to 4-byte boundaries to match lwIP makefsdata output. */
#define INDEX_NAME_LEN 12
#define NOT_FOUND_NAME_LEN 12

static const unsigned char data__404_html[] =
"/404.html\0\0\0"
"HTTP/1.0 404 File not found\r\n"
"Server: HPMicro lwIP\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<!doctype html>"
"<html lang=\"en\">"
"<head>"
"<meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>404 | Rider Gateway</title>"
"<style>"
"html,body{height:100%;margin:0;background:#090a0d;color:#f7f1dd;font-family:Impact,'Arial Narrow',sans-serif}"
"body{display:grid;place-items:center;background:radial-gradient(circle at 50% 20%,#2b0707 0,#090a0d 44%,#030406 100%)}"
".box{width:min(620px,86vw);padding:34px;border:1px solid #b71c1c;background:linear-gradient(135deg,#18110f,#090a0d);box-shadow:0 0 40px #ff1b1b55;clip-path:polygon(0 0,93% 0,100% 16%,100% 100%,7% 100%,0 84%)}"
"h1{font-size:clamp(42px,10vw,96px);margin:0;color:#ff1d1d;text-shadow:0 0 22px #ff1d1d}"
"p{font:600 18px/1.5 Arial,sans-serif;color:#d9d2bf;margin:14px 0 24px}"
"a{display:inline-block;color:#111;background:#f7c948;padding:12px 20px;text-decoration:none;font:800 14px Arial,sans-serif;letter-spacing:.08em;text-transform:uppercase}"
"</style>"
"</head>"
"<body><main class=\"box\"><h1>404</h1><p>Route not armed. Return to the Rider Gateway main console.</p><a href=\"/\">Return Home</a></main></body>"
"</html>";

static const unsigned char data__index_html[] =
"/index.html\0"
"HTTP/1.0 200 OK\r\n"
"Server: HPMicro lwIP\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<!doctype html>"
"<html lang=\"en\">"
"<head>"
"<meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>HPM Rider Gateway</title>"
"<style>"
":root{--bg:#07080b;--panel:#121319;--ink:#f8f2dc;--muted:#b9b2a1;--red:#f31b2b;--red2:#8d101a;--gold:#f6c84b;--cyan:#59f0ff;--green:#65ff8e;--line:#343743}"
"*{box-sizing:border-box}html{min-height:100%;background:#07080b}body{min-height:100%;margin:0;color:var(--ink);font-family:'Trebuchet MS','Arial Narrow',Arial,sans-serif;background:radial-gradient(circle at 72% 20%,#541018 0,#1b090c 24%,transparent 48%),radial-gradient(circle at 18% 82%,#1d3140 0,transparent 34%),linear-gradient(135deg,#05060a 0,#0c0e13 46%,#17090a 100%);overflow-x:hidden}"
"body:before{content:'';position:fixed;inset:0;pointer-events:none;background:linear-gradient(90deg,rgba(255,255,255,.035) 1px,transparent 1px),linear-gradient(rgba(255,255,255,.025) 1px,transparent 1px);background-size:42px 42px,42px 42px;mask-image:linear-gradient(to bottom,rgba(0,0,0,.75),transparent 78%)}"
"body:after{content:'';position:fixed;inset:0;pointer-events:none;background:repeating-linear-gradient(0deg,rgba(255,255,255,.035) 0 1px,transparent 1px 4px);mix-blend-mode:soft-light;opacity:.35}"
".wrap{width:min(1180px,92vw);margin:0 auto;padding:28px 0 44px}.top{display:flex;justify-content:space-between;align-items:center;gap:18px;margin-bottom:28px}.brand{display:flex;align-items:center;gap:14px;font-weight:900;text-transform:uppercase;letter-spacing:.16em}.mark{width:44px;height:44px;position:relative;background:conic-gradient(from 45deg,var(--red),var(--gold),var(--red),#280508,var(--red));clip-path:polygon(50% 0,100% 28%,86% 100%,14% 100%,0 28%);box-shadow:0 0 28px rgba(243,27,43,.6)}.mark:after{content:'';position:absolute;inset:12px;background:#07080b;clip-path:inherit}.nav{display:flex;gap:10px;flex-wrap:wrap}.nav span{border:1px solid #3b3f4e;padding:8px 11px;color:var(--muted);font-size:12px;letter-spacing:.12em;text-transform:uppercase;background:rgba(9,10,13,.7)}"
".hero{display:grid;grid-template-columns:minmax(0,1.1fr) 420px;gap:34px;align-items:center;min-height:620px}.copy{position:relative;z-index:1}.kicker{font-size:13px;color:var(--cyan);font-weight:900;letter-spacing:.28em;text-transform:uppercase;margin-bottom:16px}.title{font-family:Impact,'Arial Black',sans-serif;font-size:clamp(58px,10vw,146px);line-height:.82;margin:0;text-transform:uppercase;letter-spacing:0;text-shadow:0 0 26px rgba(243,27,43,.55),6px 7px 0 #280405}.title span{display:block;color:var(--red)}.lead{max-width:650px;margin:24px 0 28px;color:#ddd5bd;font-size:clamp(17px,2vw,22px);line-height:1.48}.actions{display:flex;gap:14px;flex-wrap:wrap}.btn{display:inline-flex;align-items:center;gap:10px;padding:15px 20px;border:0;text-decoration:none;text-transform:uppercase;letter-spacing:.1em;font-weight:900;font-size:14px;clip-path:polygon(0 0,92% 0,100% 32%,100% 100%,8% 100%,0 68%)}.btn.primary{background:linear-gradient(135deg,var(--gold),#ffef9c);color:#15110a;box-shadow:0 0 30px rgba(246,200,75,.35)}.btn.ghost{background:#14161d;color:var(--ink);border:1px solid #424756}.deck{position:relative;height:520px;display:grid;place-items:center}.belt{position:relative;width:min(400px,88vw);aspect-ratio:1;border-radius:50%;background:radial-gradient(circle at 50% 50%,#071016 0 22%,#f6c84b 23% 27%,#15110a 28% 31%,#f31b2b 32% 45%,#5b0710 46% 58%,#0a0b0f 59%);box-shadow:0 0 60px rgba(243,27,43,.62),inset 0 0 80px rgba(0,0,0,.8);animation:pulse 2.8s ease-in-out infinite}.belt:before,.belt:after{content:'';position:absolute;inset:44px;border:2px solid rgba(255,255,255,.2);border-radius:50%;clip-path:polygon(50% 0,100% 32%,80% 100%,20% 100%,0 32%)}.belt:after{inset:112px;background:linear-gradient(135deg,#e4fff9,#59f0ff 48%,#0e4d5b);box-shadow:0 0 36px rgba(89,240,255,.85);clip-path:polygon(50% 0,100% 38%,82% 100%,18% 100%,0 38%)}.slash{position:absolute;width:112%;height:22px;background:linear-gradient(90deg,transparent,#ff1b2b 18%,#f6c84b 50%,#ff1b2b 82%,transparent);transform:rotate(-28deg);box-shadow:0 0 30px rgba(255,27,43,.72)}.card{position:absolute;background:rgba(13,14,19,.86);border:1px solid #414654;backdrop-filter:blur(4px);box-shadow:0 18px 38px rgba(0,0,0,.35);padding:15px 16px;min-width:190px;clip-path:polygon(0 0,94% 0,100% 18%,100% 100%,6% 100%,0 82%)}.card b{display:block;color:var(--gold);font-size:12px;letter-spacing:.16em;text-transform:uppercase}.card span{display:block;margin-top:8px;font:900 24px/1 Arial,sans-serif;color:var(--green)}.card.one{top:32px;right:0}.card.two{left:-10px;bottom:48px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:14px;margin-top:10px}.tile{min-height:132px;padding:18px;border:1px solid #373b48;background:linear-gradient(150deg,rgba(255,255,255,.06),rgba(255,255,255,.015));clip-path:polygon(0 0,93% 0,100% 20%,100% 100%,7% 100%,0 80%)}.tile h2{margin:0 0 12px;font-size:14px;letter-spacing:.16em;text-transform:uppercase;color:var(--cyan)}.tile p{margin:0;color:var(--muted);line-height:1.45;font-size:14px}.meter{height:8px;margin-top:18px;background:#08090d;border:1px solid #333947;overflow:hidden}.meter i{display:block;height:100%;background:linear-gradient(90deg,var(--red),var(--gold),var(--green));box-shadow:0 0 18px rgba(246,200,75,.5)}.footer{display:flex;justify-content:space-between;gap:18px;margin-top:28px;color:#817b70;font-size:12px;letter-spacing:.12em;text-transform:uppercase}@keyframes pulse{0%,100%{filter:brightness(1);transform:scale(1)}50%{filter:brightness(1.24);transform:scale(1.025)}}@media(max-width:900px){.hero{grid-template-columns:1fr;min-height:0}.deck{height:390px}.grid{grid-template-columns:1fr}.top{align-items:flex-start;flex-direction:column}.card.one{right:8px}.card.two{left:8px;bottom:18px}.footer{flex-direction:column}.title{text-shadow:0 0 22px rgba(243,27,43,.55),3px 4px 0 #280405}}"
"</style>"
"</head>"
"<body>"
"<main class=\"wrap\">"
"<header class=\"top\"><div class=\"brand\"><i class=\"mark\"></i><span>HPM Rider Gateway</span></div><nav class=\"nav\"><span>lwIP</span><span>RTL8211</span><span>Flash XIP</span></nav></header>"
"<section class=\"hero\"><div class=\"copy\"><div class=\"kicker\">HTTP server armed</div><h1 class=\"title\">Henshin<span>Network</span></h1><p class=\"lead\">The HPM5E31 board is serving this page directly from its embedded lwIP HTTP stack. Static page assets are compiled into the firmware image, so the flash_xip build keeps this console available after power cycling.</p><div class=\"actions\"><a class=\"btn primary\" href=\"/\">Reboot Check</a><a class=\"btn ghost\" href=\"/404.html\">404 Route</a></div></div><div class=\"deck\"><div class=\"slash\"></div><div class=\"belt\"></div><div class=\"card one\"><b>Driver Core</b><span>ONLINE</span></div><div class=\"card two\"><b>Link Mode</b><span>HTTP:80</span></div></div></section>"
"<section class=\"grid\"><article class=\"tile\"><h2>MAC + PHY</h2><p>HPM ENET MAC talks to the RTL8211 PHY, then lwIP owns the TCP/IP stack above it.</p><div class=\"meter\"><i style=\"width:88%\"></i></div></article><article class=\"tile\"><h2>Firmware Page</h2><p>This HTML and CSS live in fsdata_custom.c and are linked into the final firmware image.</p><div class=\"meter\"><i style=\"width:96%\"></i></div></article><article class=\"tile\"><h2>Next Step</h2><p>Replace static text with CGI or SSI later when sensor, encoder, or board status data needs to be shown live.</p><div class=\"meter\"><i style=\"width:64%\"></i></div></article></section>"
"<footer class=\"footer\"><span>HPM5E31 lwIP HTTP Server</span><span>Rider-style embedded console</span></footer>"
"</main>"
"</body>"
"</html>";

const struct fsdata_file file__404_html[] = { {
    file_NULL,
    data__404_html,
    data__404_html + NOT_FOUND_NAME_LEN,
    sizeof(data__404_html) - NOT_FOUND_NAME_LEN - 1,
    FS_FILE_FLAGS_HEADER_INCLUDED,
} };

const struct fsdata_file file__index_html[] = { {
    file__404_html,
    data__index_html,
    data__index_html + INDEX_NAME_LEN,
    sizeof(data__index_html) - INDEX_NAME_LEN - 1,
    FS_FILE_FLAGS_HEADER_INCLUDED,
} };

#define FS_ROOT file__index_html
#define FS_NUMFILES 2
