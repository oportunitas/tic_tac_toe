#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <time.h> 
#define game_server 5
#define server 3
#define x_player 1
#define o_player 2
#define any_player 4

#define mark_success 0
#define mark_selffill 10
#define mark_oppfill 15
#define mark_badnum 20

#define COMPUTER 1 
#define HUMAN 2 
#define SIDE 3 
#define COMPUTERMOVE 'O' 
#define HUMANMOVE 'X' 

// Throws error and exits program
void throwError() {
    printf("💀 error encountered during execution, exiting program...\n");
    exit(EXIT_FAILURE);
}

struct mesg_buffer { 
	long type; 
	char value[256]; 
} msg; 

char board[3][3] = { 0 };

long int cli_type;

int fill_count = 0;

// Clears terminal (! clear)
void clearTerminal() {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return;
    } else {
        char* argv[] = { "clear", NULL };
        execvp("clear", argv);
    }
}

int receiveGameStream() {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return 1;
    } else {
        key_t key; 
        int msgid; 
        key = ftok("tictactoe", 65); 
        msgid = msgget(key, 0666 | IPC_CREAT); 
        
		int rcv_ret = msgrcv(msgid, &msg, sizeof(msg), cli_type, 0); 
		if (rcv_ret == -1) {
			perror("msgrcv");
		}

		if (msg.value[0] == '!') {
			msgctl(msgid, IPC_RMID, NULL); 
			return 0;
		}

		printf("%s", msg.value); 
    }

	return EXIT_FAILURE;
}

void listenToGame() {
	pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return;
    } else {
		while (true) {
			int stream_stat = receiveGameStream();
			if (stream_stat == -1) break;
		}
	}
}

void sendMessage(int msg_type, char* msg_value) {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return;
    } else {
        key_t key; 
        int msgid; 
        key = ftok("tictactoe", 65); 
        msgid = msgget(key, 0666 | IPC_CREAT); 
        msg.type = msg_type; 
		strncpy(msg.value, msg_value, 128);
        msgsnd(msgid, &msg, sizeof(msg), 0); 
    }
}

void regSendMessage(int msg_type, char* msg_value) {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return;
    } else {
        key_t key; 
        int msgid; 
        key = ftok("tictactoe", 65); 
        msgid = msgget(key, 0666 | IPC_CREAT); 
        msg.type = msg_type; 
		strncpy(msg.value, msg_value, 128);
        msgsnd(msgid, &msg, sizeof(msg), 0); 

        exit(EXIT_SUCCESS);
    }
}

void seqSendMessage(int msg_type, char* msg_value) {
    key_t key; 
    int msgid; 
    key = ftok("tictactoe", 65); 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    msg.type = msg_type; 
    strncpy(msg.value, msg_value, 128);
    msgsnd(msgid, &msg, sizeof(msg), 0); 
}

int c_cache = 0;

int confirmPlayerToServer() {
	char msg_to_server[128];
	snprintf(msg_to_server, sizeof(msg_to_server), "%li", cli_type);
	seqSendMessage(server, msg_to_server);
	
    key_t key; 
    int msgid; 
    key = ftok("tictactoe", 65); 
    msgid = msgget(key, 0666 | IPC_CREAT); 

    msgrcv(msgid, &msg, sizeof(msg), any_player, 0); 
    while (atoi(msg.value) == c_cache) {
        msgrcv(msgid, &msg, sizeof(msg), any_player, IPC_NOWAIT); 
    }

    msgctl(msgid, IPC_RMID, NULL); 
    return atoi(msg.value);
}

char* awaitMessage(int msg_type) {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
        return "str";
    } else {
        key_t key; 
        int msgid; 
        key = ftok("tictactoe", 65); 
        msgid = msgget(key, 0666 | IPC_CREAT); 

        msg.type = msg_type;
        msgrcv(msgid, &msg, sizeof(msg), msg_type, 0);  
        return msg.value;
    }

    return "str";
}

char* seqAwaitForMessage(int msg_type) {
	key_t key; 
	int msgid; 
	key = ftok("tictactoe", 65); 
	msgid = msgget(key, 0666 | IPC_CREAT); 

	msg.type = msg_type;
	msgrcv(msgid, &msg, sizeof(msg), msg_type, 0); 
	msgctl(msgid, IPC_RMID, NULL); 
	return msg.value;
}

void initializeGame() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            board[i][j] = ' ';
        }
    }

	printf("<-----TicTacToe----->\n");
	printf("                     \n");
	printf("available player type\n");
	printf("        X | 1        \n");
	printf("        O | 2        \n");
	printf("  enter your choice  \n");
	printf("     [1/2]: ");
	scanf(" %li", &cli_type);
    int res;

	while (true) {
        clearTerminal();
        if (cli_type < 1 || cli_type > 2) {
            printf("please input number between 1 and 2\n\n");
            goto printing;
        }

		res = confirmPlayerToServer();
		if (res == 100 || res == 400) break;

		if (cli_type == x_player) {
			printf("'X' has already been taken, choose another type\n\n");
		} else if (cli_type == o_player) {
			printf("'O' has already been taken, choose another type\n\n");
		}

    printing:
        printf("<-----TicTacToe----->\n");
        printf("                     \n");
        printf("available player type\n");
        printf("        X | 1        \n");
        printf("        O | 2        \n");
		printf("  enter your choice  \n");
		printf("     [1/2]: ");
		scanf(" %li", &cli_type);
	}

    clearTerminal();
    if (res == 100) {
        printf("waiting for others to join...\n"); 
        if (cli_type == x_player) {
            printf("   you play as 'X'   \n");
        } else if (cli_type == o_player) {
            printf("   you play as 'O'   \n");
        }
    }
    while (true) {
        char* state = awaitMessage(cli_type);
        if (state[0] == '+') {
            key_t key; 
            int msgid; 
            key = ftok("tictactoe", 65); 
            msgid = msgget(key, 0666 | IPC_CREAT); 
            msgctl(msgid, IPC_RMID, NULL);
            return;
        }
    }
}

void showInstructions() 
{ 
	printf("----Game Started!----\n"); 
    printf("                     \n");
    printf("    |-----------|    \n"); 
	printf("    | 1 | 2 | 3 |    \n"); 
	printf("    |-----------|    \n"); 
	printf("    | 4 | 5 | 6 |    \n"); 
	printf("    |-----------|    \n"); 
	printf("    | 7 | 8 | 9 |    \n"); 
    printf("    |-----------|    \n"); 
    printf("                     \n");
    printf("ℹ️  select the cell number to choose a cell.\n");
    printf("⏩ press enter to continue...");
    while (getchar() != '\n');
    while (getchar() != '\n');
    return;
} 

int cache;

int rowCrossed() 
{ 
	for (int i = 0; i < SIDE; i++) { 
		if (board[i][0] == board[i][1] 
			&& board[i][1] == board[i][2] 
			&& board[i][0] != ' ') 
			return 1; 
	} 
	return 0; 
} 

int columnCrossed() 
{ 
	for (int i = 0; i < SIDE; i++) { 
		if (board[0][i] == board[1][i] 
			&& board[1][i] == board[2][i] 
			&& board[0][i] != ' ') 
			return 1; 
	} 
	return 0; 
} 

int diagonalCrossed() 
{ 
	if ((board[0][0] == board[1][1] 
		&& board[1][1] == board[2][2] 
		&& board[0][0] != ' ') 
		|| (board[0][2] == board[1][1] 
			&& board[1][1] == board[2][0] 
			&& board[0][2] != ' ')) 
		return 1; 

	return 0; 
} 

int gameOver() { 
	return (
		rowCrossed() || 
		columnCrossed() || 
		diagonalCrossed()
	); 
} 

void showBoard() 
{ 
    clearTerminal();

    printf("-------------------Game Ongoing!-------------------\n"); 
    printf("                         :                         \n");
    printf("    |-----------|        :        |-----------|    \n"); 
    printf("    | 1 | 2 | 3 |        :        | %c | %c | %c | \n", board[0][0], board[0][1], board[0][2]); 
    printf("    |-----------|        :        |-----------|    \n"); 
    printf("    | 4 | 5 | 6 |        :        | %c | %c | %c | \n", board[1][0], board[1][1], board[1][2]); 
    printf("    |-----------|        :        |-----------|    \n"); 
    printf("    | 7 | 8 | 9 |        :        | %c | %c | %c | \n", board[2][0], board[2][1], board[2][2]); 
    printf("    |-----------|        :        |-----------|    \n"); 
    printf("                         :                 \n");
    printf("ℹ️  use these cell        :   \n");
    printf("   numbers to choose.    : \n"); 
    printf("                         : \n"); 
} 

char symbol[3] = { '0', 'X', 'O' };

void mark();

void waitNext() {
    char opp_name;
    if (cli_type == x_player) {
        printf("     you play as 'X'     :");
        opp_name = 'O';
    } else if (cli_type == o_player) {
        printf("     you play as 'O'     :");
        opp_name = 'X';
    }

    char reply[128];
    snprintf(reply, sizeof(reply), "%i", cache);

    printf("     now is not your turn! \n");
    if (fill_count == 9) {
        printf("the game resulted in a draw\n");
        exit(EXIT_SUCCESS);
    }

    while (cache == atoi(reply)) {
        strcpy(reply, seqAwaitForMessage(cli_type));
    }
    printf("drawing board\n");

    int row = (atoi(reply) - 1) / 3;
    int col = (atoi(reply) - 1) % 3;

    board[row][col] = opp_name;
    fill_count += 1;
    showBoard();

    if (atoi(reply) >= 100) {
        printf("\nyou (%c) have won the game! congratulations!\n", symbol[cli_type]);
        exit(EXIT_SUCCESS);
    }

    if (fill_count == 9) {
        printf("the game resulted in a draw\n");
        exit(EXIT_SUCCESS);
    }
    mark();
}

void mark() {
    char opp_name[2048];
    if (cli_type == x_player) {
        printf("     you play as 'X'     :");
        strcpy(opp_name, "O");
    } else if (cli_type == o_player) {
        printf("     you play as 'O'     :");
        strcpy(opp_name, "X");
    }

    if (gameOver()) {
        printf("\n\nthe opponent (%s) have won the game\n", opp_name);
        exit(EXIT_SUCCESS);
    }

    if (fill_count == 9) {
        printf("the game resulted in a draw\n");
        exit(EXIT_SUCCESS);
    }

    int cell_num;
    printf("     enter cell number: ");
    scanf(" %i", &cell_num);
    printf("cell num: %i\n", cell_num);

    char request[128];
    snprintf(request, sizeof(request), "%i", cell_num);
    regSendMessage(server, request);
    printf("sent: %s\n", request);
    
    char* reply = seqAwaitForMessage(cli_type);
    printf("reply: %i from %li\n", atoi(reply), cli_type);

    if (atoi(reply) == mark_badnum) {
        showBoard();
        printf("bad input, choose cell number 1-9\n");
        mark();
    } else if (atoi(reply) == mark_selffill) {
        showBoard();
        printf("choose another cell, its filled by %c\n", symbol[cli_type]);
        mark();
    } else if (atoi(reply) == mark_oppfill) {
         showBoard();
        printf("choose another cell, its filled by %s\n", opp_name);
        mark();
    } else {
        int row = (atoi(reply) - 1) / 3;
        int col = (atoi(reply) - 1) % 3;

        board[row][col] = symbol[cli_type];
        fill_count += 1;
        cache = atoi(reply);
        showBoard();
        waitNext();
    }
}

int main() {
    cache = 0;

    clearTerminal();
	initializeGame();
    clearTerminal();
    showBoard();

    if (cli_type == 1) mark();
    else waitNext();

    return 0; 
}
