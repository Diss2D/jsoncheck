# jsoncheck

Вариант №12 — Валидатор JSON
Консольная программа на C11 с использованием библиотеки cJSON для проверки синтаксиса JSON с указанием места ошибки и форматированного вывода.

## Сборка
ОС: Ubuntu 24.04 LTS.

Установка зависимостей:
```bash
sudo apt update
xargs -a build-deps.txt sudo apt install -y
```

Сборка программы:
```bash
cmake -S . -B build
cmake --build build
```


## Запуск
Проверка синтаксиса JSON-файла:

```bash
./build/jsoncheck validate example.json
```

При корректном JSON программа выводит:

```text
JSON is valid
```

При обнаружении синтаксической ошибки программа сообщает номер строки, столбца и байтовое смещение ошибки.


Форматирование JSON-файла:

```bash
./build/jsoncheck format example.json
```
Команда проверяет синтаксис JSON и выводит форматированный
документ с отступами и переносами строк.

Для сохранения результата в отдельный файл:

```bash
./build/jsoncheck format example.json > formatted.json
```
Если JSON содержит синтаксическую ошибку, программа сообщает
номер строки, столбца и байтовое смещение ошибки в stderr.

Для выбора режима форматирования можно использовать параметры
командной строки:

```bash
./build/jsoncheck format --pretty example.json
./build/jsoncheck format --compact example.json
```

Параметр `--pretty` выводит JSON с отступами и переносами строк.
Параметр `--compact` выводит JSON без лишних пробелов и переносов.

Дополнительные команды:

```bash
./build/jsoncheck --help
./build/jsoncheck --version
```

Коды завершения:
- 0 — успешное выполнение;
- 1 — ошибка проверки, форматирования или чтения файла;
- 2 — неправильные аргументы командной строки.

## Конфигурация

Программа поддерживает конфигурационный файл:

`/etc/jsoncheck/jsoncheck.conf`

Файл содержит настройку режима форматирования JSON:

```ini
format=pretty
```

Доступные значения:

- `pretty` — форматированный вывод с отступами;
- `compact` — компактный вывод JSON.

Пустые строки и строки, начинающиеся с `#`, игнорируются.

При отсутствии конфигурационного файла используется режим `pretty`.

Параметры командной строки `--pretty` и `--compact` имеют приоритет над настройками конфигурационного файла.

При обнаружении некорректного параметра конфигурации программа выводит
сообщение об ошибке и возвращает код завершения 1.

Конфигурационный файл будет автоматически устанавливаться в `/etc/jsoncheck/` при установке deb-пакета.

## Сборка deb-пакета

Для сборки установочного пакета необходима Ubuntu 24.04 LTS.

Установка зависимостей сборки:

```bash
sudo apt update
xargs -a build-deps.txt sudo apt install -y
```

Сборка deb-пакета:

```bash
./build_deb.sh
```

Сценарий выполняет выпускную сборку программы через CMake,
определяет зависимости с помощью dpkg-shlibdeps
и создаёт установочный пакет в каталоге `dist/`.

Проверка пакета:

```bash
lintian dist/*.deb
```

## Установка

Установка пакета в Ubuntu:

```bash
sudo apt install "./dist/jsoncheck_$(cat VERSION)_$(dpkg --print-architecture).deb"
```

Необходимые библиотеки времени выполнения устанавливаются
автоматически через apt.

Проверка установленной программы:

```bash
jsoncheck --version
jsoncheck --help
man jsoncheck
```

Проверка синтаксиса JSON:

```bash
jsoncheck validate example.json
```

Форматирование JSON:

```bash
jsoncheck format example.json
jsoncheck format --compact example.json
```

Исполняемый файл устанавливается в `/usr/bin/jsoncheck`,
а конфигурация — в `/etc/jsoncheck/jsoncheck.conf`.

Удаление программы с сохранением конфигурации:

```bash
sudo apt remove jsoncheck
```

Полное удаление программы и конфигурации:

```bash
sudo apt purge jsoncheck
```

## HTTP-сервер

В пр №3 добавлен HTTP-сервер на библиотеке libmicrohttpd.
Сервер использует существующий модуль jsoncore для проверки
и форматирования JSON.

Запуск:

```bash
APP_API_TOKEN=local-test ./build/jsoncheck serve
```

По умолчанию используется порт 8080.
Изменить порт можно через переменную окружения APP_PORT:

```bash
APP_PORT=9090 APP_API_TOKEN=local-test ./build/jsoncheck serve
```

Доступные HTTP-адреса:

|Метод | Адрес         | Назначение                 |
|------|---------------|----------------------------|
| GET  | /health       | Проверка работоспособности |
| GET  | /ready        | Проверка готовности        |
| GET  | /version      | Версия, имя хоста и имя сервиса |
| POST | /api/validate | Проверка синтаксиса JSON   |
| POST | /api/format   | Форматирование JSON        |

Для POST-запросов необходим заголовок X-Api-Token,
значение которого сравнивается с APP_API_TOKEN.
Токен не передаётся в исходном коде и
не хранится в репозитории.

Пример проверки JSON:

```bash
curl -H "X-Api-Token: local-test" \
  --data-binary '{"name":"demo"}' \
  http://localhost:8080/api/validate
```

Пример компактного форматирования:

```bash
curl -H "X-Api-Token: local-test" \
  --data-binary '{"name": "demo"}' \
  "http://localhost:8080/api/format?mode=compact"
```

Для форматирования доступны режимы pretty (по умолчанию)
и compact.

Максимальный размер тела POST-запроса — 1 МиБ.
При ошибках сервер возвращает соответствующие HTTP-коды,
в том числе 400, 401, 405 и 413.

Сервер корректно обрабатывает SIGINT и SIGTERM,
завершая работу с кодом 0.

### Контейнеризация Docker

Для ПР №3 подготовлен Docker-образ на основе Ubuntu 24.04.

Образ устанавливает готовый Debian-пакет jsoncheck и его
зависимости через apt, повторная компиляция программы
внутри Docker не выполняется.

Сборка Debian-пакета:

```bash
./build_deb.sh
```

Сборка Docker-образа:

```bash
sudo docker build \
  --build-arg APP_VERSION="$(cat VERSION)" \
  -t jsoncheck:local .
```

Запуск контейнера:

```bash
sudo docker run -d \
  --name jsoncheck-test \
  -p 8080:8080 \
  -e APP_API_TOKEN=local-test \
  jsoncheck:local
```

Проверка работоспособности:

```bash
curl http://localhost:8080/health
sudo docker ps
sudo docker logs jsoncheck-test
```

Программа работает от непривилегированного пользователя
с UID 10001. Docker автоматически проверяет адрес
/health через HEALTHCHECK.

Остановка контейнера:

```bash
sudo docker stop --timeout 3 jsoncheck-test
```

Удаление остановленного контейнера:

```bash
sudo docker rm jsoncheck-test
```

Размер локального образа по выводу Docker:
127 МБ использования диска, 32,4 МБ содержимого.

Образ проверен локально: HTTP API, автоматическая
проверка работоспособности, запуск без прав root,
корректное завершение по SIGTERM и повторный запуск.

### Публикация Docker-образа

В пр №3 в GitHub Actions добавлен этап
deploy, который выполняется при отправке тега вида v*.

После успешных этапов build, test и package конвейер:

1. Скачивает готовый Debian-пакет из артефактов package.
2. Собирает Docker-образ на основе Ubuntu 24.04.
3. Запускает контейнер и выполняет smoke-тесты HTTP API.
4. Авторизуется в Docker Hub через секреты GitHub Actions.
5. Публикует образ с двумя тегами: версией программы
   и коротким хешем Git-коммита.

Для публикации используются секреты DOCKERHUB_USERNAME
и DOCKERHUB_TOKEN, их значения не хранятся в репозитории.

Репозиторий Docker Hub:
https://hub.docker.com/r/diss2d/jsoncheck

После публикации релиза v0.3.0 образ можно загрузить:

```bash
sudo docker pull diss2d/jsoncheck:0.3.0
```

Запуск опубликованного образа:

```bash
sudo docker run -d \
  --name jsoncheck \
  -p 8080:8080 \
  -e APP_API_TOKEN=local-test \
  diss2d/jsoncheck:0.3.0
```

Проверка:

```bash
curl http://localhost:8080/health
sudo docker ps
```

Используется публичный Docker-репозиторий, поэтому
для загрузки образа авторизация не требуется.

## Тестирование и непрерывная интеграция

В практической работе №2 была реализована система
автоматизированного тестирования и непрерывной интеграции.

### Автоматизированное тестирование

Модульные тесты расположены в `tests/test_core.c`
и проверяют основные функции библиотеки `jsoncore`:
- проверку синтаксиса JSON;
- обработку некорректных и граничных данных;
- форматирование JSON;
- определение строки и столбца ошибки;
- обработку UTF-8 и больших документов.

Дополнительно реализованы интеграционные тесты командной строки.

Для запуска тестов используется CTest, проверка ошибок памяти
осуществляется с помощью AddressSanitizer и UndefinedBehaviorSanitizer.

Покрытие строк модуля `core` контролируется утилитой gcovr.
Минимальный допусимый уровень покрытия составляет 70%, при локальной
проверке достигнуто покрытие в 97%.

### GitHub Actions

Конвейер описан в `.github/workflows/ci.yml` и содержит
следующие задачи:

1. `build` — сборка с `-Werror` и анализ кода cppcheck.
2. `test` — запуск CTest с санитайзерами, проверка покрытия.
3. `package` — сборка deb-пакета и проверка lintian.
4. `release` — публикация GitHub Release с deb-пакетом.
5. `deploy` — сборка, проверка и публикация Docker-образа.

Задачи build, test и package выполняются последовательно,
задачи release и deploy зависят от успешного выполнения package.

Публикация релиза выполняется при отправке тега версии
или ручном запуске конвейера с указанием тега.
Публикация Docker-образа выполняется только для тегов версии.

Каждый следующий этап зависит от успешного выполнения предыдущего.

Конвейер запускается при отправке изменений в ветки,
создании Pull Request в main и отправке тегов `v*`.

Результаты тестирования сохраняются в формате JUnit,
а готовый deb-пакет доступен в артефактах GitHub Actions.

### Защита основной ветки

Для ветки `main` настроены обязательные проверки build,
test и package, слияние Pull Request запрещено,
если хотя бы одна из обязательных проверок не пройдена.

Работа механизма подтверждена с помощью отдельной
демонстрационной ветки с утечкой памяти.

Демонстрационный Pull Request:
https://github.com/Diss2D/jsoncheck/pull/6

### Релиз программы

Версия программы определяется файлом `VERSION`.

При отправке Git-тега `v*` сценарий сборки проверяет
совпадение номера версии с тегом.

После успешной сборки, тестирования и пакетирования
GitHub Actions автоматически создаёт GitHub Release
с готовым deb-пакетом.

Страница релизов:
https://github.com/Diss2D/jsoncheck/releases

## Развёртывание в Kubernetes

Для ПР №4 используется локальный кластер Kubernetes
на базе kind.

Проверенное окружение:
- Ubuntu 24.04 LTS (WSL 2);
- Docker 29.1.3;
- kind v0.33.0;
- kubectl v1.37.0.

Инструкции по установке инструментов:
- https://kind.sigs.k8s.io/docs/user/quick-start/
- https://kubernetes.io/docs/tasks/tools/install-kubectl-linux/

### Создание кластера

```bash
mkdir -p ~/.kube

sudo kind create cluster --name tsp \
  --kubeconfig "$HOME/.kube/config" \
  --wait 5m

sudo chown "$(id -u):$(id -g)" ~/.kube/config
chmod 600 ~/.kube/config

kubectl get nodes
```

Узел кластера должен находиться в состоянии Ready.

### Манифесты

В каталоге `k8s/` расположены:
- `namespace.yaml` — пространство имён jsoncheck;
- `configmap.yaml` — настройки приложения;
- `deployment.yaml` — развёртывание двух реплик;
- `service.yaml` — Service типа ClusterIP.

Приложение получает APP_PORT из ConfigMap,
а APP_API_TOKEN — из Secret через переменные окружения.

### Развёртывание приложения

Создание пространства имён:

```bash
kubectl apply -f k8s/namespace.yaml
```

Создание Secret:

```bash
read -rsp "API token: " API_TOKEN
echo

kubectl -n jsoncheck create secret generic jsoncheck-secret \
  --from-literal=APP_API_TOKEN="$API_TOKEN"

unset API_TOKEN
```

Применение манифестов:

```bash
kubectl apply -f k8s/

kubectl -n jsoncheck rollout status \
  deployment/jsoncheck --timeout=180s

kubectl -n jsoncheck get pods
kubectl -n jsoncheck get services
```

Должны появиться две готовые реплики приложения
в состоянии Running и Service типа ClusterIP.

Для доступа к сервису с локальной машины:

```bash
kubectl -n jsoncheck port-forward svc/jsoncheck 8080:80
```

В другом терминале:

```bash
curl -fsS http://localhost:8080/health
curl -fsS http://localhost:8080/ready
curl -fsS http://localhost:8080/version
```

### Пробы работоспособности

ReadinessProbe проверяет адрес /ready каждые 5 секунд,
при неуспешной проверке под исключается из маршрутизации
запросов до восстановления готовности.

LivenessProbe проверяет адрес /health каждые 10 секунд,
при достижении порога неуспешных проверок Kubernetes
перезапускает контейнер.

Контейнер работает от непривилегированного пользователя
с UID 10001. В Deployment задан runAsNonRoot - true.

Используется стратегия RollingUpdate с параметрами
maxUnavailable: 0 и maxSurge: 1.

### Ресурсы

Потребление ресурсов измерено с помощью docker stats
на опубликованном образе jsoncheck:0.3.0.

В течение 60 секунд выполнялись запросы к /api/validate
с JSON-массивом из 10000 элементов размером 58890 байт.

Результаты:
- успешных запросов: 1119;
- неуспешных запросов: 0;
- CPU без нагрузки: 0,00%;
- CPU под нагрузкой: 3,43–4,19%;
- память до нагрузки: 3,215 МиБ;
- максимальное измеренное потребление памяти: 11,37 МиБ.

Для одной реплики выбраны следующие ресурсы:

| Ресурс | Requests | Limits |
|--------|----------|--------|
| CPU    | 50m      | 250m   |
| Память | 32Mi     | 64Mi   |

Значения выбраны с запасом относительно измеренного
потребления для обработки более интенсивной нагрузки.

### Обновление без простоя

Для обновления приложения используется стратегия RollingUpdate:

- `maxUnavailable: 0` — сохранение доступности реплик;
- `maxSurge: 1` — создание одной дополнительной реплики;
- `minReadySeconds: 5` — минимальное время готовности новой реплики;
- `terminationGracePeriodSeconds: 30` — время на корректное завершение пода;
- `preStop: sleep 10` — задержка остановки контейнера для обновления
  маршрутизации Service.

При обновлении с версии 1.0.0 до 1.0.1 все 120 HTTP-запросов
выполнены успешно без ошибок.

## Лицензия

Проект распространяется под лицензией MIT.
Полный текст находится в файле [LICENSE](LICENSE).

## Практические работы
- ПР №1: создание программы и deb-пакета (выполнено).
- ПР №2: автоматизированное тестирование, CI и релиз программы (выполнено).
- ПР №3: контейнеризация и публикация Docker-образа (выполнено).
- ПР №4: развёртывание и сопровождение в Kubernetes (в процессе).
