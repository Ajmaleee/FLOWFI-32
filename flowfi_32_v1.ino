/*
  ESP32 Contact Card Captive Portal
  ---------------------------------
  - Broadcasts a WiFi AP with a chosen SSID
  - Redirects ALL DNS + unknown HTTP requests to itself so phones
    auto-open the captive portal login page (Android "Sign in to network",
    iOS "Cancel/Join" popup) without the user typing an IP.
  - Serves a mobile-first, neumorphic ("soft UI") contact page with
    tactile raised/pressed buttons for: Call, SMS, Email, LinkedIn,
    Instagram, GitHub. All styling is inline CSS with system fonts only
    (no external font/CDN requests) since visitors are on an isolated
    AP with no internet access.
  - Includes a WhatsApp box: visitor types their name, taps the button,
    and it opens WhatsApp (wa.me) with a pre-filled "Hii, I'm <name>"
    message to YOUR number — so you know who to save when you get the ping.

  Board: written for ESP32 Arduino core 3.x (WiFi.h / DNSServer.h / WebServer.h
  are all built-in, no extra libraries to install).
  Tested target: any standard ESP32 module (WROOM-32 etc.) with default
  4MB flash / default partition scheme — this sketch is tiny (~15KB of code,
  compiles to roughly 850KB-1MB with WiFi+WebServer libs), so it fits
  comfortably even on the smallest common ESP32 boards. If you're using a
  board with less flash (e.g. an ESP32-C3 Super Mini variant with 2MB),
  it will still fit, just pick "Minimal SPIFFS" or "No OTA" partition
  scheme in Tools > Partition Scheme if you ever add more code later.
*/

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// ---------------- WiFi AP settings ----------------
const char* ssid     = "ajmaleee Contact Card";   // Name shown when scanning WiFi
const char* password = "";                  // "" = open network (recommended for
                                             // auto-popup). If you set one, it
                                             // must be 8+ chars.

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

// ---------------- EDIT YOUR DETAILS HERE ----------------
const char* MY_NAME        = "Ajmaleee__";
// Leave INITIALS empty ("") to auto-generate from MY_NAME, or set your own
// (e.g. "AB") if the auto version doesn't look right.
const char* INITIALS       = "AJ";
const char* PHONE_NUMBER   = "+919495050785";      // used for tel: and sms:
const char* WHATSAPP_NUMBER= "919495050785";       // digits only, country code, no + no spaces
const char* EMAIL          = "ajmalsworkshop@gmail.com";
const char* LINKEDIN_URL   = "https://linkedin.com/in/ajmaleee";
const char* INSTAGRAM_URL  = "https://instagram.com/ajmaleee__";
const char* GITHUB_URL     = "https://github.com/ajmaleee";
// ----------------------------------------------------------

// Builds up to 2 initials from MY_NAME (e.g. "Jane Doe" -> "JD") if
// INITIALS was left empty above.
String getInitials() {
  if (strlen(INITIALS) > 0) return String(INITIALS);
  String name = String(MY_NAME);
  String out = "";
  bool atWordStart = true;
  for (size_t i = 0; i < name.length() && out.length() < 2; i++) {
    char c = name[i];
    if (c == ' ') { atWordStart = true; continue; }
    if (atWordStart) { out += (char)toupper(c); atWordStart = false; }
  }
  return out.length() ? out : "?";
}

String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#E7E2D9">
<title>%NAME% - Contact</title>
<style>
  :root{
    --bg:#E7E2D9;
    --shadow-dark:#C3BAAA;
    --shadow-light:#FFFFFF;
    --ink:#2C2820;
    --ink-soft:#84796A;
    --accent:#4B7A6B;
    --accent-dark:#3A6255;
    --radius-lg:26px;
    --radius-md:18px;
    --radius-sm:12px;
  }
  *{box-sizing:border-box;}
  html,body{margin:0;padding:0;}
  body{
    min-height:100vh;
    background:var(--bg);
    color:var(--ink);
    font-family:ui-rounded,"SF Pro Rounded","Segoe UI Rounded",-apple-system,"Segoe UI",Roboto,sans-serif;
    display:flex;
    justify-content:center;
    align-items:flex-start;
    padding:28px 16px 40px;
  }
  .card{
    width:100%;
    max-width:400px;
  }
  .avatar-wrap{
    display:flex;
    justify-content:center;
    margin-bottom:14px;
  }
  .avatar{
    width:88px;height:88px;border-radius:50%;
    background:var(--bg);
    display:flex;align-items:center;justify-content:center;
    font-size:30px;font-weight:700;letter-spacing:1px;
    color:var(--accent-dark);
    box-shadow:8px 8px 16px var(--shadow-dark),-8px -8px 16px var(--shadow-light);
  }
  h1{
    text-align:center;
    font-size:clamp(20px,5.5vw,26px);
    font-weight:800;
    margin:0 0 6px;
    letter-spacing:.2px;
  }
  .eyebrow{
    display:flex;align-items:center;justify-content:center;gap:7px;
    margin:0 auto 26px;
    width:max-content;
    padding:8px 16px;
    border-radius:999px;
    font-size:12px;
    color:var(--ink-soft);
    box-shadow:inset 4px 4px 8px var(--shadow-dark),inset -4px -4px 8px var(--shadow-light);
  }
  .dot{width:7px;height:7px;border-radius:50%;background:var(--accent);flex:none;}
  .stack{display:flex;flex-direction:column;gap:14px;margin-bottom:26px;}
  .btn{
    display:flex;align-items:center;gap:12px;
    width:100%;
    padding:14px 18px;
    border-radius:var(--radius-md);
    border:none;
    background:var(--bg);
    color:var(--ink);
    font-size:15px;font-weight:600;
    text-decoration:none;
    cursor:pointer;
    box-shadow:7px 7px 14px var(--shadow-dark),-7px -7px 14px var(--shadow-light);
    transition:box-shadow .12s ease, transform .12s ease;
    opacity:0;
    animation:rise .5s ease forwards;
  }
  .btn:active{
    box-shadow:inset 5px 5px 10px var(--shadow-dark),inset -5px -5px 10px var(--shadow-light);
    transform:scale(.99);
  }
  .btn:focus-visible{outline:2px solid var(--accent);outline-offset:3px;}
  .chip{
    flex:none;width:38px;height:38px;border-radius:12px;
    display:flex;align-items:center;justify-content:center;
    font-size:17px;color:#fff;
  }
  .chip.call{background:#5B8C6E;}
  .chip.sms{background:#5B7A9C;}
  .chip.mail{background:#B0654F;}
  .chip.li{background:#4C6FA0;font-weight:800;font-size:14px;}
  .chip.ig{background:#B05E82;}
  .chip.gh{background:#4A473F;}
  .label{flex:1;}
  .chev{color:var(--ink-soft);font-size:16px;}
  .panel{
    border-radius:var(--radius-lg);
    padding:20px;
    box-shadow:inset 6px 6px 12px var(--shadow-dark),inset -6px -6px 12px var(--shadow-light);
  }
  .panel p{
    margin:0 0 12px;
    font-size:13px;
    color:var(--ink-soft);
    text-align:center;
  }
  .field{
    width:100%;
    padding:14px 16px;
    margin-bottom:14px;
    border:none;border-radius:var(--radius-sm);
    background:var(--bg);
    color:var(--ink);
    font-size:15px;
    box-shadow:inset 4px 4px 9px var(--shadow-dark),inset -4px -4px 9px var(--shadow-light);
  }
  .field::placeholder{color:var(--ink-soft);}
  .field:focus-visible{outline:2px solid var(--accent);outline-offset:2px;}
  .wa-btn{
    display:flex;align-items:center;justify-content:center;gap:10px;
    width:100%;padding:15px;border:none;border-radius:var(--radius-sm);
    background:var(--accent);color:#fff;font-size:15px;font-weight:700;
    cursor:pointer;
    box-shadow:6px 6px 12px var(--shadow-dark),-3px -3px 8px var(--shadow-light);
    transition:box-shadow .12s ease, transform .12s ease;
  }
  .wa-btn:active{
    box-shadow:inset 4px 4px 9px var(--accent-dark),inset -3px -3px 7px #6a9a8a;
    transform:scale(.99);
  }
  .wa-btn:focus-visible{outline:2px solid var(--accent-dark);outline-offset:3px;}
  footer{text-align:center;font-size:11px;color:var(--ink-soft);margin-top:22px;}

  @keyframes rise{
    from{opacity:0;transform:translateY(10px);}
    to{opacity:1;transform:translateY(0);}
  }
  .stack .btn:nth-child(1){animation-delay:.05s;}
  .stack .btn:nth-child(2){animation-delay:.10s;}
  .stack .btn:nth-child(3){animation-delay:.15s;}
  .stack .btn:nth-child(4){animation-delay:.20s;}
  .stack .btn:nth-child(5){animation-delay:.25s;}
  .stack .btn:nth-child(6){animation-delay:.30s;}

  @media (min-width:460px){
    .card{max-width:420px;}
    .avatar{width:96px;height:96px;font-size:32px;}
  }

  @media (prefers-reduced-motion:reduce){
    .btn{animation:none;opacity:1;}
  }
</style>
</head>
<body>
<main class="card">
  <div class="avatar-wrap">
    <div class="avatar">%INITIALS%</div>
  </div>
  <h1>%NAME%</h1>
  <div class="eyebrow"><span class="dot"></span>You're connected &middot; tap to reach out</div>

  <nav class="stack">
    <a class="btn" href="tel:%PHONE%">
      <span class="chip call">&#128222;</span>
      <span class="label">Call Me</span>
      <span class="chev">&#8250;</span>
    </a>
    <a class="btn" href="sms:%PHONE%">
      <span class="chip sms">&#128172;</span>
      <span class="label">Send SMS</span>
      <span class="chev">&#8250;</span>
    </a>
    <a class="btn" href="mailto:%EMAIL%">
      <span class="chip mail">&#9993;</span>
      <span class="label">Email Me</span>
      <span class="chev">&#8250;</span>
    </a>
    <a class="btn" href="%LINKEDIN%" target="_blank" rel="noopener">
      <span class="chip li">in</span>
      <span class="label">LinkedIn</span>
      <span class="chev">&#8250;</span>
    </a>
    <a class="btn" href="%INSTAGRAM%" target="_blank" rel="noopener">
      <span class="chip ig">&#128247;</span>
      <span class="label">Instagram</span>
      <span class="chev">&#8250;</span>
    </a>
    <a class="btn" href="%GITHUB%" target="_blank" rel="noopener">
      <span class="chip gh">&#128025;</span>
      <span class="label">GitHub</span>
      <span class="chev">&#8250;</span>
    </a>
  </nav>

  <section class="panel">
    <p>Type your name so I know who to save when you say hii</p>
    <label for="uname" style="position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0 0 0 0);">Your name</label>
    <input id="uname" class="field" type="text" placeholder="Your name" autocomplete="name">
    <button class="wa-btn" onclick="sendWA()">
      <span>&#128994;</span> Say Hii on WhatsApp
    </button>
  </section>

  <footer>Powered by ESP32</footer>
</main>
<script>
function sendWA(){
  var n = document.getElementById('uname').value.trim();
  if(!n){ alert('Please enter your name first'); return; }
  var msg = "Hii, I'm " + n;
  var url = "https://wa.me/%WANUMBER%?text=" + encodeURIComponent(msg);
  window.open(url, '_blank');
}
</script>
</body>
</html>
)rawliteral";

  page.replace("%NAME%", MY_NAME);
  page.replace("%INITIALS%", getInitials());
  page.replace("%PHONE%", PHONE_NUMBER);
  page.replace("%EMAIL%", EMAIL);
  page.replace("%LINKEDIN%", LINKEDIN_URL);
  page.replace("%INSTAGRAM%", INSTAGRAM_URL);
  page.replace("%GITHUB%", GITHUB_URL);
  page.replace("%WANUMBER%", WHATSAPP_NUMBER);
  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// Any request the OS doesn't recognize as a known captive-portal probe
// gets redirected back to root -> this is what pops the "Sign in to
// network" / captive portal browser automatically.
void handleNotFound() {
  server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  // Known OS captive-portal check URLs — serve the page directly on these too
  server.on("/generate_204", handleRoot);                 // Android
  server.on("/gen_204", handleRoot);                      // Android
  server.on("/hotspot-detect.html", handleRoot);          // iOS / macOS
  server.on("/library/test/success.html", handleRoot);    // iOS
  server.on("/ncsi.txt", handleRoot);                      // Windows
  server.on("/connecttest.txt", handleRoot);               // Windows
  server.on("/fwlink", handleRoot);                         // Windows

  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Captive portal started");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}