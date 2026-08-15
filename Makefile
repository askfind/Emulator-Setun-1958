.PHONY: all clean run debug release help

# ============================================================================
# Конфигурация
# ============================================================================
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11
LDFLAGS = -lm

# Исходные файлы
SOURCES = setun1958_vm.c command.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = setun1958emu

# Дополнительные опции для разных режимов
#RELEASE_FLAGS = -O2 -DNDEBUG
DEBUG_FLAGS = -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address
WARNING_FLAGS = -Wpedantic -Wshadow -Wcast-align -Wconversion \
                -Wsign-conversion -Wmissing-prototypes \
                -Wstrict-prototypes -Wmissing-declarations

# ============================================================================
# Цели по умолчанию
# ============================================================================
all: release

# ============================================================================
# Режимы сборки
# ============================================================================
release: CFLAGS += $(RELEASE_FLAGS)
release: $(EXECUTABLE)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(EXECUTABLE)

# ============================================================================
# Сборка
# ============================================================================
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✅ Сборка завершена: $(EXECUTABLE)"
	@echo "📦 Размер: $$(du -h $@ | cut -f1)"
	@echo "🔧 Режим: $(if $(findstring -g,$(CFLAGS)),DEBUG,RELEASE)"

%.o: %.c | check_sources
	$(CC) -c $(CFLAGS) $(WARNING_FLAGS) $< -o $@
	@echo "  Компиляция: $<"

# ============================================================================
# Проверка исходников
# ============================================================================
check_sources:
	@for src in $(SOURCES); do \
		if [ ! -f "$$src" ]; then \
			echo "❌ Ошибка: Файл $$src не найден!"; \
			exit 1; \
		fi; \
	done

# ============================================================================
# Запуск
# ============================================================================
run: $(EXECUTABLE)
	@echo "🚀 Запуск: $(EXECUTABLE)"
	./$(EXECUTABLE)

run-debug: debug
	@echo "🐛 Запуск в режиме отладки (с санитайзером)"
	./$(EXECUTABLE)

# ============================================================================
# Вспомогательные цели
# ============================================================================
clean:
	@echo "🧹 Очистка..."
	rm -f $(OBJECTS) $(EXECUTABLE)
	@echo "✅ Готово"

distclean: clean
	@echo "🧹 Глубокая очистка..."
	rm -f *.o *.out core vgcore.*
	@echo "✅ Готово"

# ============================================================================
# Анализ кода
# ============================================================================
check: $(SOURCES)
	@echo "🔍 Проверка кода..."
	@for src in $(SOURCES); do \
		echo "  Проверка $$src"; \
		$(CC) -fsyntax-only $(CFLAGS) $(WARNING_FLAGS) $$src || exit 1; \
	done
	@echo "✅ Синтаксис OK"

cppcheck:
	@echo "🔍 Запуск cppcheck..."
	cppcheck --enable=all --suppress=missingIncludeSystem $(SOURCES)

# ============================================================================
# Профилирование и отладка
# ============================================================================
profile: CFLAGS += -pg -O2
profile: clean $(EXECUTABLE)
	@echo "📊 Сборка с профилированием"
	./$(EXECUTABLE)
	gprof $(EXECUTABLE) gmon.out > profile.txt
	@echo "✅ Профиль сохранён в profile.txt"

valgrind: debug
	@echo "🔧 Запуск Valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --verbose ./$(EXECUTABLE)

# ============================================================================
# Документация и информация
# ============================================================================
info:
	@echo "📋 Информация о проекте:"
	@echo "  Компилятор: $(CC)"
	@echo "  Исходники: $(SOURCES)"
	@echo "  Объекты: $(OBJECTS)"
	@echo "  Исполняемый: $(EXECUTABLE)"
	@echo ""
	@echo "📌 Доступные цели:"
	@echo "  make         - Собрать релизную версию"
	@echo "  make debug   - Собрать с отладочной информацией"
	@echo "  make release - Собрать оптимизированную версию"
	@echo "  make run     - Запустить программу"
	@echo "  make clean   - Удалить объектные файлы"
	@echo "  make distclean - Полная очистка"
	@echo "  make check   - Проверить синтаксис"
	@echo "  make valgrind - Проверка утечек памяти"
	@echo "  make profile - Профилирование"
	@echo "  make cppcheck - Статический анализ"

help: info

# ============================================================================
# Правило для зависимостей (автоматическое создание)
# ============================================================================
dep:
	$(CC) -MM $(SOURCES) > .depend

-include .depend

# ============================================================================
# Защита от случайного удаления
# ============================================================================
.DELETE_ON_ERROR: