#include "core.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "FAIL: %s (line %d)\n", #condition, __LINE__);                         \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

int main(void) {
    size_t error_offset = 0;

    // 1. Корректный JSON-объект
    CHECK(jsoncheck_validate("{\"name\":\"test\",\"count\":2}", &error_offset));

    // 2. Корректный JSON-массив
    CHECK(jsoncheck_validate("[true,false,null]", NULL));

    // 3. Некорректный синтаксис JSON
    CHECK(!jsoncheck_validate("{\"name\":}", &error_offset));

    // 4. Посторонний текст после JSON
    CHECK(!jsoncheck_validate("{\"name\":1} trailing", NULL));

    // 5. Пустой ввод
    CHECK(!jsoncheck_validate("", NULL));

    // 6. Нулевой указатель
    CHECK(!jsoncheck_validate(NULL, NULL));

    // 7. Форматирование некорректного JSON
    CHECK(jsoncheck_format("{invalid}", true) == NULL);

    // 8. компактное форматирование
    char *compact = jsoncheck_format("{ \"a\" : 1 }", false);
    CHECK(compact != NULL);

    if (compact != NULL) {
        CHECK(strcmp(compact, "{\"a\":1}") == 0);
        jsoncheck_free(compact);
    }

    // 9. Форматирование с отступами
    char *pretty = jsoncheck_format("{\"a\":1}", true);
    CHECK(pretty != NULL);

    if (pretty != NULL) {
        CHECK(strchr(pretty, '\n') != NULL);
        jsoncheck_free(pretty);
    }

    // 10. Определение строки и столбца
    size_t line = 0;
    size_t column = 0;

    jsoncheck_error_location("a\nb\nc", 4, &line, &column);
    CHECK(line == 3);
    CHECK(column == 1);

    // 11. Корректный подлсчёт символов UTF-8
    jsoncheck_error_location("Жx", strlen("Ж"), &line, &column);
    CHECK(line == 1);
    CHECK(column == 2);

    // 12. Обработка большого JSON-документа
    char large_json[65539];

    large_json[0] = '"';
    memset(large_json + 1, 'a', sizeof(large_json) - 3);
    large_json[sizeof(large_json) - 2] = '"';
    large_json[sizeof(large_json) - 1] = '\0';

    CHECK(jsoncheck_validate(large_json, NULL));

    if (failures != 0) {
        fprintf(stderr, "Failed checks: %d\n", failures);
        return 1;
    }
    printf("All core checks passed\n");
    return 0;
}
