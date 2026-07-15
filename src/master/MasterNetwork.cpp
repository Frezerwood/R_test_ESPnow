#include "master/MasterNetwork.h"
#include <WiFi.h>

// Локальные константы, убранные из глобальной видимости main
static const char *AP_SSID = "KineticWall";
static const char *AP_PASS = "12345678";

MasterNetwork::MasterNetwork() : _server(80) {}

void MasterNetwork::begin()
{
    setupWiFi();
    setupEspNow();

    registerHttpRoutes();

    _server.begin();

    Serial.println("[HTTP] Server started");
}

void MasterNetwork::handle()
{
    _server.handleClient();
}

void MasterNetwork::setupWiFi()
{
    WiFi.mode(WIFI_AP_STA);

    // bool ok = WiFi.softAP(AP_SSID, AP_PASS);
    bool ok = WiFi.softAP(
        AP_SSID,
        AP_PASS,
        ESPNOW_CHANNEL);

    Serial.printf(
        "AP channel: %d\n",
        WiFi.channel());

    Serial.printf("AP start: %s\n", ok ? "OK" : "FAIL");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
}

void MasterNetwork::setupEspNow()
{
    // WiFi.mode(WIFI_AP_STA); // Комбинированный режим для AP + ESP-NOW

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("[Network] Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(MasterNetwork::onDataSent);

    // Регистрируем 4 слейва из config.h в качестве пиров
    for (int i = 0; i < 4; i++)
    {
        if (!addPeer(SLAVE_MACS[i]))
        {
            Serial.printf("[Network] Failed to add slave %d\n", i);
        }
    }
}

bool MasterNetwork::addPeer(const uint8_t mac[6])
{
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    return (esp_now_add_peer(&peer) == ESP_OK);
}

void MasterNetwork::onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    // Вывод статуса доставки пакета в шину
    Serial.printf("[ESP-NOW] Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

bool MasterNetwork::sendServoMaskCommand(uint8_t slaveId, uint8_t servoMask, uint16_t holdMs)
{
    if (slaveId >= 4)
        return false;

    ServoCommand cmd = {servoMask, holdMs};
    esp_err_t result = esp_now_send(SLAVE_MACS[slaveId], (uint8_t *)&cmd, sizeof(cmd));
    return (result == ESP_OK);
}

void MasterNetwork::broadcastMatrix(uint8_t m0, uint8_t m1, uint8_t m2, uint8_t m3, uint16_t hold)
{
    FullMatrixCommand cmd;
    cmd.masks[0] = m0;
    cmd.masks[1] = m1;
    cmd.masks[2] = m2;
    cmd.masks[3] = m3;
    cmd.holdMs = hold;

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastAddress, (uint8_t *)&cmd, sizeof(cmd));
}

void MasterNetwork::setCell(uint8_t row, uint8_t col, uint16_t holdMs)
{
    if (row >= 4 || col >= 4)
        return;

    // Используем карту матрицы из matrix_map.h для трансляции глобальных координат
    CellMap cell = MATRIX_MAP[row * 4 + col];
    sendServoMaskCommand(cell.slaveId, cell.mask, holdMs);
}

void MasterNetwork::registerAnimationCallbacks(
    std::function<void()> onWave,
    std::function<void()> onRows,
    std::function<void()> onExplosion,
    std::function<void()> onSlave2)
{
    _onWaveStart = onWave;
    _onRowsStart = onRows;
    _onExplosionStart = onExplosion;
    _onSlave2Start = onSlave2;
}

void MasterNetwork::registerHttpRoutes()
{
    _server.on("/", HTTP_GET, [this]()
               {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Kinetic Wall</title>

<style>
body{
    font-family:Arial;
    background:#121212;
    color:white;
    text-align:center;
    margin-top:40px;
}

.container{
    width:360px;
    margin:auto;
}

button{
    width:160px;
    height:55px;
    margin:8px;
    border:none;
    border-radius:12px;
    font-size:18px;
    background:#2d2d2d;
    color:white;
}

button:hover{
    background:#444;
}

.stop{
    background:#b00020;
}

.slider{
    width:320px;
}
</style>

</head>

<body>

<div class="container">

<h1>Kinetic Wall</h1>

<h2>Patterns</h2>

<button onclick="runPattern('wave')">Wave</button>
<button onclick="runPattern('rows')">Rows</button>

<button onclick="runPattern('explosion')">Explosion</button>
<button onclick="runPattern('slave2')">Slave2 Test</button>

<br>

<button class="stop" onclick="stopAll()">STOP</button>

<h2>Speed</h2>

<input
    type="range"
    min="20"
    max="200"
    value="100"
    class="slider"
    id="speedSlider"
    onchange="setSpeed()"
>

<div id="speedValue">100%</div>

<script>

async function runPattern(name)
{
    await fetch('/api/pattern',{
        method:'POST',
        headers:{
            'Content-Type':'text/plain'
        },
        body:name
    });
}

async function stopAll()
{
    await fetch('/api/stop',{
        method:'POST'
    });
}

async function setSpeed()
{
    const value =
        document.getElementById('speedSlider').value;

    document.getElementById(
        'speedValue'
    ).innerText = value + '%';

    await fetch('/api/speed',{
        method:'POST',
        headers:{
            'Content-Type':'text/plain'
        },
        body:value
    });
}

</script>

</div>

</body>
</html>
)rawliteral";

        _server.send(200, "text/html", html); });

    _server.on("/api/pattern", HTTP_POST, [this]()
               {
        String body = _server.arg("plain");

        if(body == "wave" && _onWaveStart)
            _onWaveStart();

        else if(body == "rows" && _onRowsStart)
            _onRowsStart();

        else if(body == "explosion" && _onExplosionStart)
            _onExplosionStart();

        else if(body == "slave2" && _onSlave2Start)
            _onSlave2Start();

        _server.send(200,"text/plain","OK"); });

    _server.on("/api/stop", HTTP_POST, [this]()
               {
        broadcastMatrix(0,0,0,0,0);
        _server.send(200,"text/plain","STOP"); });

    _server.on("/api/speed", HTTP_POST, [this]()
               {
        String value = _server.arg("plain");

        Serial.printf(
            "[WEB] Speed changed to %s%%\n",
            value.c_str()
        );

        _server.send(200,"text/plain","OK"); });
}