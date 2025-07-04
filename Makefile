# Makefile para a Simulação da Barbearia de Hilzer

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11 -g

# Nome do executável final
TARGET = barbershop_sim

# Arquivos fonte
SOURCES = main.c barbershop.c utils.c

# Arquivos objeto (derivados dos arquivos fonte)
OBJECTS = $(SOURCES:.c=.o)

# Regra padrão: limpar e depois compilar tudo
all: clean $(TARGET)

# Regra para linkar o executável a partir dos arquivos objeto
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Regra genérica para compilar arquivos .c em .o
# Depende do .c correspondente e do header principal para recompilar se o header mudar
%.o: %.c barbershop.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos compilados e o executável
clean:
	@echo "Limpando arquivos compilados..."
	rm -f $(TARGET) $(OBJECTS)

# Regra para compilar e executar a simulação
# Como 'run' depende de 'all', ele também executará 'clean' primeiro.
run: all
	./$(TARGET)

# Declara alvos que não são arquivos, para evitar conflitos e otimizar o make
.PHONY: all clean run
