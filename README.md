# ESP8266 Barrier Controller (ENC28J60) with NetPing IO v2/v4 API compatibility

Промышленный сетевой контроллер шлагбаума для ESP8266 NodeMCU + ENC28J60.

## Функции
- Управление сухим контактом OPEN через реле.
- Два концевика (OPEN_LIMIT, CLOSE_LIMIT) с `INPUT_PULLUP`.
- Режимы открытия: `PULSE` и `HOLD_UNTIL_LIMIT`.
- State machine без блокирующих `delay()`.
- HTTP Web UI: Status / Control / Logic settings / Network settings.
- NetPing-совместимый endpoint `/io.cgi`.
- JSON API `/api/status`, `/api/open`, `/api/close`, `/api/reset_error`, `/api/config`.
- Конфиг в LittleFS (`/config.json`).
- Кольцевой буфер логов.

## Распиновка
- ENC28J60 SCK -> D5(GPIO14)
- ENC28J60 MISO -> D6(GPIO12)
- ENC28J60 MOSI -> D7(GPIO13)
- ENC28J60 CS -> D2(GPIO4)
- Relay IN1 -> D0(GPIO16)
- OPEN_LIMIT -> D1(GPIO5)
- CLOSE_LIMIT -> RX(GPIO3)

## Сборка
```bash
pio run
```

## Загрузка
```bash
pio run -t upload
pio run -t uploadfs
```

## NetPing совместимость
- `/io.cgi?io1=0`
- `/io.cgi?io1=1`
- `/io.cgi?io1=f`
- `/io.cgi?io1=f,5`
- `/io.cgi?io1`

## Авторизация
Поддержаны:
- trusted IP
- shared token (`?token=...`)
- HTTP Basic auth (`login/password`, пароль хранится как SHA1 hash)
