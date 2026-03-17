# ESP8266 Barrier Controller (ENC28J60) with NetPing IO compatibility

Полностью рабочий firmware-проект для ESP8266 NodeMCU + ENC28J60 (Ethernet), собираемый в PlatformIO.

## Главное исправление зависимостей PlatformIO
Используется корректная библиотека из registry:

```ini
lib_deps =
  jandrassy/EthernetENC
```

В проекте используется только EthernetENC стек (`#include <EthernetENC.h>`).

## Функции
- Управление шлагбаумом через реле как сухой контакт COM/NO.
- Концевики OPEN_LIMIT и CLOSE_LIMIT (`INPUT_PULLUP`) с программным debounce.
- Контроль аварийной ситуации: оба концевика активны дольше `dual_limit_error_ms`.
- State machine без `delay()`:
  `BOOT`, `IDLE_UNKNOWN`, `IDLE_CLOSED`, `OPENING`, `OPENED_HOLD`, `CLOSING`, `ERROR`.
- Режимы открытия:
  - `PULSE`
  - `HOLD_UNTIL_LIMIT`
- Web UI: STATUS / CONTROL / LOGIC SETTINGS / NETWORK SETTINGS.
- NetPing-совместимый endpoint `/io.cgi`.
- JSON API:
  - `GET /api/status`
  - `POST /api/open`
  - `POST /api/close`
  - `POST /api/reset_error`
  - `GET /api/config`
  - `POST /api/config`
- Хранение конфигурации в LittleFS (`/config.json`).
- Кольцевой буфер логов формата: `timestamp, event_type, source, message`.

## Распиновка
- ENC28J60 SCK -> D5 (GPIO14)
- ENC28J60 MISO -> D6 (GPIO12)
- ENC28J60 MOSI -> D7 (GPIO13)
- ENC28J60 CS -> D2 (GPIO4)
- Relay IN1 -> D0 (GPIO16)
- OPEN_LIMIT -> D1 (GPIO5)
- CLOSE_LIMIT -> RX (GPIO3)

## Сборка
```bash
pio run
```

## Прошивка
```bash
pio run -t upload
```

## Заливка LittleFS
```bash
pio run -t uploadfs
```

## NetPing compatibility
Поддержаны запросы:
- `/io.cgi?io1=0`
- `/io.cgi?io1=1`
- `/io.cgi?io1=f`
- `/io.cgi?io1=f,5`
- `/io.cgi?io1`

Формат ответов:
- `io_result('ok')`
- `io_result('error')`

## Auth
- Trusted IP
- Shared token (`?token=...`)
- HTTP Basic auth (`login/password`, хранится SHA1 hash)
