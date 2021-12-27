ARQUIVOS: application

all: $(ARQUIVOS)

application: application.c
	gcc -Wall -o app application.c data_link.c helper.c receiver.c sender.c

.PHONY: clean


clean:
	rm -f app