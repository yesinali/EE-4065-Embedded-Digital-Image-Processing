#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

const char* ssid = "godlessrose";
const char* password = "UsRiot_191";

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; font-family: Arial; background-color: #333; color: white; }
    h2 { color: #f2f2f2; }
    img { border: 5px solid #555; max-width: 100%; }
    input[type=range] { width: 300px; }
    .controls { margin: 20px; padding: 20px; background: #444; display: inline-block; border-radius: 10px; }
  </style>
</head>
<body>
  <h2>ESP32 Resizing Stream</h2>
  <div class="controls">
    <label>Scale: <span id="scaleVal">1.0</span>x</label><br><br>
    <input type="range" min="0.1" max="2.0" step="0.1" value="1.0" id="scaleSlider" oninput="updateLabel(this.value)">
  </div>
  <br>
  <img src="" id="streamView">
  <script>
    const view = document.getElementById('streamView');
    const slider = document.getElementById('scaleSlider');
    const label = document.getElementById('scaleVal');
    let isProcessing = false;

    function updateLabel(val) {
      label.innerHTML = val;
    }

    function loadNextImage() {
      if (isProcessing) return;
      isProcessing = true;
      
      const scale = slider.value;
      const url = "/capture?scale=" + scale + "&r=" + Math.random();

      const img = new Image();
      img.onload = function() {
        view.src = this.src;
        isProcessing = false;
        requestAnimationFrame(loadNextImage); 
      };
      img.onerror = function() {
        isProcessing = false;
        setTimeout(loadNextImage, 500);
      };
      img.src = url;
    }
    window.onload = loadNextImage;
  </script>
</body>
</html>
)rawliteral";

uint8_t* resizeImage(uint8_t* src, int srcW, int srcH, int newW, int newH) {
  uint8_t* dest = (uint8_t*) ps_calloc(newW * newH, sizeof(uint8_t));
  if (!dest) return NULL;

  float x_ratio = (float)srcW / newW;
  float y_ratio = (float)srcH / newH;

  for (int y = 0; y < newH; y++) {
    for (int x = 0; x < newW; x++) {
      int srcX = (int)(x * x_ratio);
      int srcY = (int)(y * y_ratio);

      if (srcX >= srcW) srcX = srcW - 1;
      if (srcY >= srcH) srcY = srcH - 1;

      dest[y * newW + x] = src[srcY * srcW + srcX];
    }
  }
  return dest;
}

const int bmpHeaderSize = 54;
const int colorPaletteSize = 1024; 

void createBmpHeader(uint8_t* header, int w, int h) {
  int fileSize = bmpHeaderSize + colorPaletteSize + (w * h);
  memset(header, 0, bmpHeaderSize);
  
  header[0] = 'B'; header[1] = 'M';
  header[2] = fileSize; header[3] = fileSize >> 8; header[4] = fileSize >> 16; header[5] = fileSize >> 24;
  header[10] = bmpHeaderSize + colorPaletteSize; header[11] = (bmpHeaderSize + colorPaletteSize) >> 8; header[12] = 0; header[13] = 0;
  
  header[14] = 40; 
  header[18] = w; header[19] = w >> 8; header[20] = w >> 16; header[21] = w >> 24;
  header[22] = -h; header[23] = -h >> 8; header[24] = -h >> 16; header[25] = -h >> 24; 
  header[26] = 1; 
  header[28] = 8; 
  header[30] = 0; 
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t capture_handler(httpd_req_t *req) {
  float scale = 1.0;
  char buf[100];
  if (httpd_req_get_url_query_len(req) > 0) {
      httpd_req_get_url_query_str(req, buf, sizeof(buf));
      char value[10];
      if (httpd_query_key_value(buf, "scale", value, sizeof(value)) == ESP_OK) {
          scale = atof(value);
      }
  }

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return ESP_FAIL;

  int newW = (int)(fb->width * scale);
  int newH = (int)(fb->height * scale);

  if(newW > 640) newW = 640; 
  if(newH > 480) newH = 480;
  if(newW < 10) newW = 10;

  uint8_t* resizedImg = resizeImage(fb->buf, fb->width, fb->height, newW, newH);
  esp_camera_fb_return(fb);

  if(!resizedImg) {
     httpd_resp_send_500(req);
     return ESP_FAIL;
  }

  uint8_t header[bmpHeaderSize];
  createBmpHeader(header, newW, newH);
  
  uint8_t palette[1024];
  for(int i=0; i<256; i++) {
    palette[i*4] = i;     
    palette[i*4+1] = i;   
    palette[i*4+2] = i;   
    palette[i*4+3] = 0;   
  }

  httpd_resp_set_type(req, "image/bmp");
  httpd_resp_send_chunk(req, (const char*)header, bmpHeaderSize);
  httpd_resp_send_chunk(req, (const char*)palette, 1024);
  httpd_resp_send_chunk(req, (const char*)resizedImg, newW * newH);
  httpd_resp_send_chunk(req, NULL, 0);
  
  free(resizedImg); 
  return ESP_OK;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t stream_httpd = NULL;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &capture_uri);
  }
}

void setup() {
  Serial.begin(115200);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE; 
  config.frame_size = FRAMESIZE_QVGA;        
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(psramFound()){
    config.fb_count = 2;
  } 

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error 0x%x", err);
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(WiFi.localIP());

  startCameraServer();
}

void loop() {
  delay(10000);
}