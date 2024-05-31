# Компилятор и флаги
CXX = g++
CFLAGS = -g -Wall -Wextra -pedantic

# Каталог с исходными файлами
SRCDIR = src

# Основное правило
all: startcpp.exe

# Сборка исполняемого файла
startcpp.exe: $(SRCDIR)/main.o $(SRCDIR)/liblogger.so
	$(CXX) $(CFLAGS) $(SRCDIR)/main.o -ldl -L$(SRCDIR) -llogger -o startcpp.exe

# Правило для компиляции объектных файлов
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

# Правило для создания динамической библиотеки
$(SRCDIR)/liblogger.so: $(SRCDIR)/liblogger.cpp
	$(CXX) $(CFLAGS) -shared -fPIC $(SRCDIR)/liblogger.cpp -o $(SRCDIR)/liblogger.so

# Правило очистки
.PHONY: clean
clean:
	rm -f startcpp.exe
	rm -f $(SRCDIR)/*.o $(SRCDIR)/*.so

# Ручное указание зависимостей
$(SRCDIR)/main.o: $(SRCDIR)/main.cpp $(SRCDIR)/liblogger.h
$(SRCDIR)/liblogger.o: $(SRCDIR)/liblogger.cpp $(SRCDIR)/liblogger.h
