# Анализатор трафика

## Описание

Разработанная программа считывает объем трафика, исходящего и входящего относительно
хоста, на котором запускается программа, группируя данные по адресу конечного
сервера. Из выведенной статистики можно получить:

- Количесвто входящих и исходящих пакетов
- Входящий и исходящий трафик
- Имя хоста (если имя не было выявлено, будет выведен IP адрес)

Выходная статистика выглядит следующим образом:

```console
mozilla.cloudflare-dns.com               414 packets (OUT 222    |    192 IN) traffic:   122016 [bytes] (OUT 38488    |  83528 IN)
github.com                                86 packets (OUT 37     |     49 IN) traffic:    86854 [bytes] (OUT 7689     |  79165 IN)
services.addons.mozilla.org               30 packets (OUT 16     |     14 IN) traffic:    21426 [bytes] (OUT 2175     |  19251 IN)
avatars.githubusercontent.com             71 packets (OUT 37     |     34 IN) traffic:    26242 [bytes] (OUT 5478     |  20764 IN)
contile.services.mozilla.com              21 packets (OUT 12     |      9 IN) traffic:     7599 [bytes] (OUT 1839     |   5760 IN)
push.services.mozilla.com                 34 packets (OUT 19     |     15 IN) traffic:    10776 [bytes] (OUT 3545     |   7231 IN)
```

## Установка

Для работы программы, необходимо чтобы на устройстве были установлены библиотеки Boost и Served.
Served можно установить, запустив файл served-installer.sh, который находится в корневой директории проекта.
Остальные библиотеки будут автоматически установлены в папку external.

## Использование

```console

Basic usage:
    traffic-analyzer [-hl] [-i interfaceIp] [-t executionTime] [-u updateTime]

Allowed Options:
  -h [ --help ]                        Produce help message.
  -l [ --list-interfaces ]             Print the list of interfaces.
  -i [ --ip ] arg (=127.0.0.1)         Use the specified interface.
  -t [ --exe-time ] arg (=2147483647)  Program execution time (in sec).
  -u [ --update-time ] arg (=5)        Terminal update frequency (in sec).
```

Присутствует возможность получить статистику в формате json через http-интерфейс.
При запуске программы будет выведен адрес по которому можно запросить json статистику.

```console
Use this to get statistics in JSON format: curl "http://localhost:8080/stat"
> curl "http://localhost:8080/stat"
{"hosts":[{"ip":"140.82.121.3","name":"github.com","packets":{"in":60,"out":47,"total":107},"traffic":{"in":104016,"out":9391,"total":113407}},{"ip":"18.165.122.26","name":"services.addons.mozilla.org","packets":{"in":14,"out":16,"total":30},"traffic":{"in":19251,"out":2175,"total":21426}},{"ip":"185.199.108.133","name":"avatars.githubusercontent.com","packets":{"in":64,"out":64,"total":128},"traffic":{"in":37912,"out":9133,"total":47045}},{"ip":"34.117.237.239","name":"contile.services.mozilla.com","packets":{"in":14,"out":17,"total":31},"traffic":{"in":6645,"out":2208,"total":8853}},{"ip":"34.117.65.55","name":"push.services.mozilla.com","packets":{"in":15,"out":19,"total":34},"traffic":{"in":7231,"out":3545,"total":10776}}]}
```

## Технологии

Язык программирования: `С++`

Используемые библиотеки:

- `Boost`
- [`PcapPlusPlus`](https://pcapplusplus.github.io/)
- [`Served`](http://underthehood.meltwater.com/served/)
- [`Nlohmann JSON`](https://json.nlohmann.me/)
