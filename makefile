all: test gracz bot

test: server.c
	gcc -g -Wall $^ -lpthread -lrt -lncurses -o $@

gracz: player.c
	gcc -g -Wall $^ -lpthread -lrt -lncurses -o $@

bot: bot.c
	gcc -g -Wall $^ -lpthread -lrt -lncurses -o $@

.PHONY: clean

clean:
	-rm gracz test bot
