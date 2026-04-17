#include <WiFi.h>
#include <WebServer.h>

// =========================
// PINES
// =========================
#define UP_LOCAL       18
#define DOWN_LOCAL     16
#define RST_LOCAL      22

#define UP_VISITA      19
#define DOWN_VISITA    14
#define RST_VISITA     23

#define UP_SETS        21
#define RST_SETS       17

#define BOCINA_RELE    13

// =========================
// WIFI
// =========================
const char* ssid = "TABLERO_VOLEY";
const char* password = "12345678";

WebServer server(80);

// =========================
// CONFIG
// =========================
const int PULSO_US = 80;      // un poco más ancho para más seguridad
const int RESET_MS = 10;
const int BLOQUEO_MS = 180;

const int BOCINA_MS = 400;
const bool RELE_ACTIVO_ALTO = true;

int marcadorLocal = 0;
int marcadorVisita = 0;
int marcadorSets = 0;

unsigned long ultimoComando = 0;

// =========================
// BLOQUEO
// =========================
bool bloqueado() {
  unsigned long ahora = millis();
  if (ahora - ultimoComando < BLOQUEO_MS) return true;
  ultimoComando = ahora;
  return false;
}

// =========================
// BOCINA
// =========================

void bocinaOff() {
  digitalWrite(BOCINA_RELE, RELE_ACTIVO_ALTO ? LOW : HIGH);
}

void bocinaOn() {
  digitalWrite(BOCINA_RELE, RELE_ACTIVO_ALTO ? HIGH : LOW);
}

void tocarBocina() {
  bocinaOn();
  delay(BOCINA_MS);
  bocinaOff();
}

// =========================
// ESTADO REPOSO
// =========================
void estadoReposo() {
  digitalWrite(UP_LOCAL, HIGH);
  digitalWrite(DOWN_LOCAL, HIGH);
  digitalWrite(RST_LOCAL, LOW);

  digitalWrite(UP_VISITA, HIGH);
  digitalWrite(DOWN_VISITA, HIGH);
  digitalWrite(RST_VISITA, LOW);

  // SETS (74LS90)
  digitalWrite(UP_SETS, HIGH);
  digitalWrite(RST_SETS, LOW);

  bocinaOff();
}

// =========================
// LOCAL
// =========================
void sumaLocal() {
  digitalWrite(RST_LOCAL, LOW);
  digitalWrite(DOWN_LOCAL, HIGH);

  digitalWrite(UP_LOCAL, LOW);
  delayMicroseconds(PULSO_US);
  digitalWrite(UP_LOCAL, HIGH);
}

void restaLocal() {
  digitalWrite(RST_LOCAL, LOW);
  digitalWrite(UP_LOCAL, HIGH);

  digitalWrite(DOWN_LOCAL, LOW);
  delayMicroseconds(PULSO_US);
  digitalWrite(DOWN_LOCAL, HIGH);
}

void resetLocal() {
  digitalWrite(UP_LOCAL, HIGH);
  digitalWrite(DOWN_LOCAL, HIGH);

  digitalWrite(RST_LOCAL, HIGH);
  delay(RESET_MS);
  digitalWrite(RST_LOCAL, LOW);
}

// =========================
// VISITA
// =========================
void sumaVisita() {
  digitalWrite(RST_VISITA, LOW);
  digitalWrite(DOWN_VISITA, HIGH);

  digitalWrite(UP_VISITA, LOW);
  delayMicroseconds(PULSO_US);
  digitalWrite(UP_VISITA, HIGH);
}

void restaVisita() {
  digitalWrite(RST_VISITA, LOW);
  digitalWrite(UP_VISITA, HIGH);

  digitalWrite(DOWN_VISITA, LOW);
  delayMicroseconds(PULSO_US);
  digitalWrite(DOWN_VISITA, HIGH);
}

void resetVisita() {
  digitalWrite(UP_VISITA, HIGH);
  digitalWrite(DOWN_VISITA, HIGH);

  digitalWrite(RST_VISITA, HIGH);
  delay(RESET_MS);
  digitalWrite(RST_VISITA, LOW);
}

// =========================
// SETS (74LS90)
// =========================
const int PULSO_SETS_US = 120;   // más largo porque es más sensible

void sumaSets() {
  digitalWrite(RST_SETS, LOW);

  digitalWrite(UP_SETS, LOW);
  delayMicroseconds(PULSO_SETS_US);
  digitalWrite(UP_SETS, HIGH);
}

void resetSets() {
  digitalWrite(UP_SETS, HIGH);

  digitalWrite(RST_SETS, HIGH);
  delay(RESET_MS);
  digitalWrite(RST_SETS, LOW);
}

// =========================
// RESET INICIAL
// =========================
void resetInicial() {
  estadoReposo();
  delay(200);

  resetLocal();
  delay(20);

  resetVisita();
  delay(20);

  resetSets();
  delay(20);

  marcadorLocal = 0;
  marcadorVisita = 0;
  marcadorSets = 0;

  estadoReposo();
}

// =========================
// SINCRONIZACION
// =========================
void cargarLocal(int valor) {
  resetLocal();
  delay(20);

  for (int i = 0; i < valor; i++) {
    sumaLocal();
    delay(20);
  }
}

void cargarVisita(int valor) {
  resetVisita();
  delay(20);

  for (int i = 0; i < valor; i++) {
    sumaVisita();
    delay(20);
  }
}

void cargarSets(int valor) {
  resetSets();
  delay(20);

  for (int i = 0; i < valor; i++) {
    sumaSets();
    delay(20);
  }
}

void sincronizarTablero() {
  cargarLocal(marcadorLocal);
  delay(30);

  cargarVisita(marcadorVisita);
  delay(30);

  cargarSets(marcadorSets);
  delay(30);

  estadoReposo();
}

// =========================
// RESPUESTA SIMPLE API
// =========================
void responderOK(const char* msg) {
  server.send(200, "text/plain", msg);
}

String estadoJSON() {
  String json = "{";
  json += "\"local\":" + String(marcadorLocal) + ",";
  json += "\"visita\":" + String(marcadorVisita) + ",";
  json += "\"sets\":" + String(marcadorSets);
  json += "}";
  return json;
}

// =========================
// HTML
// =========================
String paginaHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Tablero Voley</title>
  <style>
    *{ box-sizing:border-box; }

    body{
      margin:0;
      font-family:Arial, Helvetica, sans-serif;
      background:#0f172a;
      color:white;
      text-align:center;
    }

    .wrap{
      max-width:900px;
      margin:0 auto;
      padding:20px 14px 30px;
    }

    h1{
      margin:8px 0 6px;
      font-size:32px;
    }

    .sub{
      opacity:0.85;
      margin-bottom:18px;
      font-size:16px;
    }

    .grid{
      display:grid;
      grid-template-columns:1fr 1fr;
      gap:14px;
      align-items:start;
    }

    .panelSets{
      grid-column:1 / 3;
    }

    .panel{
      background:#1e293b;
      border-radius:18px;
      padding:18px;
      box-shadow:0 8px 24px rgba(0,0,0,0.25);
    }

    .panel h2{
      margin:4px 0 14px;
      font-size:30px;
    }

    .btn{
      width:100%;
      border:none;
      border-radius:16px;
      padding:22px 14px;
      margin:12px 0;
      font-size:28px;
      font-weight:bold;
      color:white;
      cursor:pointer;
      transition:transform 0.05s ease, opacity 0.15s ease;
      -webkit-tap-highlight-color: transparent;
      touch-action: manipulation;
    }

    .btn:active{
      transform:scale(0.98);
    }

    .sumar{ background:#16a34a; }
    .restar{ background:#dc2626; }
    .reset{ background:#2563eb; }

    .barra{
      margin-top:18px;
      padding:14px;
      border-radius:14px;
      background:#111827;
      font-size:18px;
      opacity:0.95;
    }

    .estado{
      margin-top:16px;
      min-height:24px;
      font-size:16px;
      opacity:0.9;
    }

    .topbtn{
      margin-top:18px;
      width:100%;
      max-width:420px;
      background:#7c3aed;
    }

    .bocina{
      margin-top:12px;
      width:100%;
      max-width:420px;
      background:#f59e0b;
    }

    .marcadorLocalBox,
    .marcadorVisitaBox,
    .marcadorSetsBox{
      margin:10px 0 18px;
      padding:14px;
      border-radius:16px;
      background:#0f172a;
      box-shadow:inset 0 0 14px rgba(0,0,0,0.35);
    }

    .marcadorLocalValor,
    .marcadorVisitaValor,
    .marcadorSetsValor{
      background:#050505;
      color:#ff3b3b;
      border-radius:14px;
      padding:14px 10px;
      font-size:68px;
      font-weight:bold;
      font-family:monospace;
      letter-spacing:8px;
      text-align:center;
      text-shadow:
        0 0 6px rgba(255, 0, 0, 0.75),
        0 0 14px rgba(255, 0, 0, 0.35);
      box-shadow:inset 0 0 14px rgba(255,0,0,0.10);
    }

    @media (max-width: 500px){
      .wrap{
        max-width:100%;
        padding:12px 8px 20px;
      }

      .grid{
        gap:10px;
      }

      .panel{
        padding:12px;
        min-width:0;
      }

      .panel h2{
        font-size:24px;
        margin:2px 0 10px;
      }

      .btn{
        font-size:20px;
        padding:16px 10px;
        margin:10px 0;
      }

      .marcadorLocalValor,
      .marcadorVisitaValor,
      .marcadorSetsValor{
        font-size:48px;
        letter-spacing:4px;
        padding:12px 6px;
      }
    }
</style>
</head>
<body>
  <div class="wrap">
    <h1>TABLERO VOLEY</h1>
    <div class="sub">Control Wi-Fi sin recarga de página</div>

    <div class="grid">
      <div class="panel">
        <h2>LOCAL</h2>

        <div class="marcadorLocalBox">
          <div class="marcadorLocalValor" id="marcadorLocalWeb">00</div>
       </div>

        <button class="btn sumar" onclick="enviar('/up_local')">+1 SUMAR</button>
        <button class="btn restar" onclick="enviar('/down_local')">-1 RESTAR</button>
        <button class="btn reset" onclick="enviar('/reset_local')">RESET</button>
      </div>

      <div class="panel">
        <h2>VISITA</h2>

        <div class="marcadorVisitaBox">
          <div class="marcadorVisitaValor" id="marcadorVisitaWeb">00</div>
        </div>

        <button class="btn sumar" onclick="enviar('/up_visita')">+1 SUMAR</button>
        <button class="btn restar" onclick="enviar('/down_visita')">-1 RESTAR</button>
        <button class="btn reset" onclick="enviar('/reset_visita')">RESET</button>
      </div>

      <div class="panel panelSets">
        <h2>SETS</h2>

       <div class="marcadorSetsBox">
          <div class="marcadorSetsValor" id="marcadorSetsWeb">00</div>
        </div>

        <button class="btn sumar" onclick="enviar('/up_sets')">+1 SET</button>
        <button class="btn reset" onclick="enviar('/reset_sets')">RESET SETS</button>
      </div>

  </div>  

<button class="btn topbtn" onclick="enviar('/reset_all')">RESET PUNTOS</button>
<button class="btn bocina" onclick="enviar('/bocina')">BOCINA</button>
<button class="btn topbtn" onclick="enviar('/actualizar')">ACTUALIZAR TABLERO</button>

<div class="barra">IP: 192.168.4.1</div>
<div class="estado" id="estado">Listo</div>
  </div>

<script>
  let ocupado = false;

  function dosDigitos(n){
    return String(n).padStart(2, '0');
  }

  async function actualizarMarcadores(){
    try {
      const res = await fetch('/estado', { cache:'no-store' });
      const data = await res.json();

      document.getElementById('marcadorLocalWeb').textContent = dosDigitos(data.local);
      document.getElementById('marcadorVisitaWeb').textContent = dosDigitos(data.visita);
      document.getElementById('marcadorSetsWeb').textContent = dosDigitos(data.sets);
    } catch(e) {
      console.log(e);
    }
  }

  async function enviar(ruta){
    if (ocupado) return;
    ocupado = true;

    const estado = document.getElementById('estado');
    estado.textContent = 'Enviando comando...';

    try {
      const res = await fetch(ruta, { method: 'GET', cache: 'no-store' });
      const txt = await res.text();
      estado.textContent = txt || 'OK';
      await actualizarMarcadores();
    } catch(e) {
      console.log(e);
      estado.textContent = 'Error de conexión';
    }

    setTimeout(() => {
      ocupado = false;
    }, 120);
  }
    actualizarMarcadores();
    setInterval(actualizarMarcadores, 1000);
  </script>
</body>
</html>
)rawliteral";
  return html;
}

// =========================
// RUTAS WEB
// =========================
void handleRoot() {
  server.send(200, "text/html", paginaHTML());
}

// LOCAL
void handleUpLocal() {
  if (!bloqueado()) {
    sumaLocal();

    if (marcadorLocal == 99) {
      marcadorLocal = 0;
    } else {
      marcadorLocal++;
    }

    Serial.println("SUMA LOCAL");
    responderOK("SUMA LOCAL");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleDownLocal() {
  if (!bloqueado()) {
    restaLocal();

    if (marcadorLocal == 0) {
      marcadorLocal = 99;
    } else {
      marcadorLocal--;
    }

    Serial.println("RESTA LOCAL");
    responderOK("RESTA LOCAL");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleResetLocal() {
  if (!bloqueado()) {
    resetLocal();
    marcadorLocal = 0;
    Serial.println("RESET LOCAL");
    responderOK("RESET LOCAL");
  } else {
    responderOK("BLOQUEADO");
  }
}

// VISITA
void handleUpVisita() {
  if (!bloqueado()) {
    sumaVisita();

    if (marcadorVisita == 99) {
      marcadorVisita = 0;
    } else {
      marcadorVisita++;
    }

    Serial.println("SUMA VISITA");
    responderOK("SUMA VISITA");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleDownVisita() {
  if (!bloqueado()) {
    restaVisita();

    if (marcadorVisita == 0) {
      marcadorVisita = 99;
    } else {
      marcadorVisita--;
    }

    Serial.println("RESTA VISITA");
    responderOK("RESTA VISITA");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleResetVisita() {
  if (!bloqueado()) {
    resetVisita();
    marcadorVisita = 0;
    Serial.println("RESET VISITA");
    responderOK("RESET VISITA");
  } else {
    responderOK("BLOQUEADO");
  }
}

// PUNTOS
void handleResetAll() {
  if (!bloqueado()) {
    resetLocal();
    delay(20);
    resetVisita();

    marcadorLocal = 0;
    marcadorVisita = 0;

    Serial.println("RESET PUNTOS");
    responderOK("RESET PUNTOS");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleUpSets() {
  if (!bloqueado()) {
    sumaSets();
    if (marcadorSets < 9) marcadorSets++;
    Serial.println("SUMA SET");
    responderOK("SUMA SET");
  } else {
    responderOK("BLOQUEADO");
  }
}

void handleResetSets() {
  if (!bloqueado()) {
    resetSets();
    marcadorSets = 0;
    Serial.println("RESET SETS");
    responderOK("RESET SETS");
  } else {
    responderOK("BLOQUEADO");
  }
}

// BOCINA
void handleBocina() {
  if (!bloqueado()) {
    tocarBocina();
    Serial.println("BOCINA");
    responderOK("BOCINA");
  } else {
    responderOK("BLOQUEADO");
  }
}
// SINCRONIZAR
void handleActualizar() {
  if (!bloqueado()) {
    sincronizarTablero();
    Serial.println("TABLERO ACTUALIZADO");
    responderOK("TABLERO ACTUALIZADO");
  } else {
    responderOK("BLOQUEADO");
  }
}

// ESTADO
void handleEstado() {
  server.send(200, "application/json", estadoJSON());
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(UP_LOCAL, OUTPUT);
  pinMode(DOWN_LOCAL, OUTPUT);
  pinMode(RST_LOCAL, OUTPUT);

  pinMode(UP_VISITA, OUTPUT);
  pinMode(DOWN_VISITA, OUTPUT);
  pinMode(RST_VISITA, OUTPUT);
  pinMode(UP_SETS, OUTPUT);
  pinMode(RST_SETS, OUTPUT);
  pinMode(BOCINA_RELE, OUTPUT);

  estadoReposo();
  delay(100);

  resetInicial();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("=== TABLERO VOLEY WIFI ===");
  Serial.print("Red: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);

  server.on("/up_local", handleUpLocal);
  server.on("/down_local", handleDownLocal);
  server.on("/reset_local", handleResetLocal);

  server.on("/up_visita", handleUpVisita);
  server.on("/down_visita", handleDownVisita);
  server.on("/reset_visita", handleResetVisita);

  server.on("/reset_all", handleResetAll);

  server.on("/up_sets", handleUpSets);
  server.on("/reset_sets", handleResetSets);

  server.on("/bocina", handleBocina);
  server.on("/actualizar", handleActualizar);
  server.on("/estado", handleEstado);

  server.begin();
  Serial.println("Servidor web iniciado");
}

// =========================
// LOOP
// =========================
void loop() {
  server.handleClient();
}