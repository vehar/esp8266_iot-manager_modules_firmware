void WIFI_init() {

  // --------------------Получаем SSDP со страницы
  HTTP.on("/ssid", HTTP_GET, []() {
    jsonWrite(configSetup, "ssid", HTTP.arg("ssid"));
    jsonWrite(configSetup, "password", HTTP.arg("password"));
    saveConfig();                 // Функция сохранения данных во Flash
    HTTP.send(200, "text/plain", "OK"); // отправляем ответ о выполнении
  });
  // --------------------Получаем SSDP со страницы
  HTTP.on("/ssidap", HTTP_GET, []() {
    jsonWrite(configSetup, "ssidAP", HTTP.arg("ssidAP"));
    jsonWrite(configSetup, "passwordAP", HTTP.arg("passwordAP"));
    saveConfig();                 // Функция сохранения данных во Flash
    HTTP.send(200, "text/plain", "OK"); // отправляем ответ о выполнении
  });


  // Попытка подключения к точке доступа
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
#ifdef led_status
  led_blink(2, 1, "on");
#endif
  byte tries = 20;
  String _ssid = jsonRead(configSetup, "ssid");
  String _password = jsonRead(configSetup, "password");
  WiFi.persistent(false);
  if (_ssid == "" && _password == "") {
    WiFi.begin();
  }
  else {
    WiFi.begin(_ssid.c_str(), _password.c_str());
  }
  // Делаем проверку подключения до тех пор пока счетчик tries
  // не станет равен нулю или не получим подключение
  while (--tries && WiFi.status() != WL_CONNECTED)
  {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("password is not correct");
      tries = 1;
      jsonWrite(optionJson, "pass_status", 1);
    }
    Serial.print(".");
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    // Если не удалось подключиться запускаем в режиме AP
    Serial.println("");
    StartAPMode();

  }
  else {
    // Иначе удалось подключиться отправляем сообщение
    // о подключении и выводим адрес IP
    Serial.println("");
    Serial.println("->WiFi connected");
#ifdef date_logging
    addFile("log.txt", GetDataDigital() + " " + GetTime() + "=>WiFi connected");
#endif
#ifdef led_status
    led_blink(2, 1, "off");
#endif
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    jsonWrite(configJson, "ip", WiFi.localIP().toString());
  }
}

bool StartAPMode() {
  Serial.println("WiFi up AP");
#ifdef date_logging
  addFile("log.txt", GetDataDigital() + " " + GetTime() + "->WiFi up AP");
#endif
  IPAddress apIP(192, 168, 4, 1);
  IPAddress staticGateway(192, 168, 4, 1);
  IPAddress staticSubnet(255, 255, 255, 0);
  // Отключаем WIFI
  WiFi.disconnect();
  // Меняем режим на режим точки доступа
  WiFi.mode(WIFI_AP);
  // Задаем настройки сети
  WiFi.softAPConfig(apIP, staticGateway, staticSubnet);
  //Включаем DNS
  //dnsServer.start(53, "*", apIP);
  // Включаем WIFI в режиме точки доступа с именем и паролем
  // хронящихся в переменных _ssidAP _passwordAP
  String _ssidAP = jsonRead(configSetup, "ssidAP");
  String _passwordAP = jsonRead(configSetup, "passwordAP");
  WiFi.softAP(_ssidAP.c_str(), _passwordAP.c_str());
  jsonWrite(configJson, "ip", apIP.toString());
#ifdef led_status
  led_blink(2, 200, "on");
#endif

  if (jsonReadtoInt(optionJson, "pass_status") != 1) {
    ts.add(WIFI, reconnecting_wifi, [&](void*) {
      Serial.println("->try find router");
#ifdef date_logging
      addFile("log.txt", GetDataDigital() + " " + GetTime() + "->try find router");
#endif
      if (RouterFind(jsonRead(configSetup, "ssid"))) {
        ts.remove(WIFI);
        WIFI_init();
        MQTT_init();
      }
    }, nullptr, true);
  }
  return true;
}


boolean RouterFind(String ssid) {
  int n = WiFi.scanComplete ();
  if (n == -2) {                       //Сканирование не было запущено, запускаем
    WiFi.scanNetworks (true, false);   //async, show_hidden
    return false;
  }
  if (n == -1) {                       //Сканирование все еще выполняется
    return false;
  }
  if (n > 0) {
    for (int i = 0; i <= n; i++) {
      if (WiFi.SSID (i) == ssid) {
        WiFi.scanDelete();
        return true;
      }
    }
    WiFi.scanDelete();
    return false;
  }
}
