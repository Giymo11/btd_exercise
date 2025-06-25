## Changelog & wichtige Infos 25.06.2025 | Aleks

### Was funktioniert
* state management
* buttons und zwischen states gehen
* auto off nach 5 Min
* QR codes
* server
* configs
* break gesture
* schritte erkennen und pause auflösen
* alles was nicht unten steht

### Was nicht geht
* vibrator: ich glaube ich krieg das nicht hin, hat mich schon sehr viel Zeit gekostet. Seit dem Umstieg auf Arduino lib mag er einfach gar nicht gehen.
* microphone: habe ich auch nicht getestet

### Code
jedes State hat 3 Schritte:
* start
* handle: returns true beim Übergang zum nächsten State
* stop 

### TODOs
- [ ] Protokollieren ins CSV und herunterladen - bitte um Unterstützung!!!!
- [ ] microphone
- [ ] vibrator/beep als alternative

### Bedienung
* Siehe Diagramme in Excalidraw :)
* Wenn man in "ti:ma" wlan ist, kommt man direkt zum zweiten QR Code fürs Config (wow) 

### Wichtiges zu beachten
* alles ist im main
* in init() wird folgendes Code ausgeführt. Dadurch wird config nicht persistiert, das müsste man dann auskommentieren nachdem man nvs einmal erased hat :)
```
ESP_ERROR_CHECK(nvs_flash_erase()); // IMPORTANT! Erase NVS at first startup, comment out to persist data
```

### Danksagung
Danke für euren Code, es hat fast alles auf Anhieb funktioniert :D



