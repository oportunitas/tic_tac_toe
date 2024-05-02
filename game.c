#include <curl/curl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h> 
#define game_server 5
#define server 3
#define x_player 1
#define o_player 2

#define mark_success 0
#define mark_selffill 10
#define mark_oppfill 15
#define mark_badnum 20

#define COMPUTER 1 
#define HUMAN 2 
#define SIDE 3 
#define COMPUTERMOVE 'O' 
#define HUMANMOVE 'X' 

struct Move { 
	int row, col; 
}; 

char player = 'x', opponent = 'o'; 
char board[3][3] = { 0 };

void showBoard() 
{ 
    system("clear");
    printf("----Game Ongoing!----\n\n"); 
    printf("    |-----------|\n"); 
    printf("    | %c | %c | %c |\n", board[0][0], board[0][1], board[0][2]); 
    printf("    |-----------|\n"); 
    printf("    | %c | %c | %c |\n", board[1][0], board[1][1], board[1][2]); 
    printf("    |-----------|\n"); 
    printf("    | %c | %c | %c |\n", board[2][0], board[2][1], board[2][2]); 
    printf("    |-----------|\n\n"); 
} 

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

// Throws error and exits program
void throwError() {
    printf("💀 error encountered during execution, exiting program...\n");
    exit(EXIT_FAILURE);
}

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

// structure for message queue 
struct mesg_buffer { 
	long type; 
	char value[256]; 
} msg; 

void stream(char* line) {
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
        strncpy(msg.value, line, 128);

        msg.type = 1; 
        msgsnd(msgid, &msg, sizeof(msg), 0); 

        msg.type = 2;
        msgsnd(msgid, &msg, sizeof(msg), 0); 
        printf("stream: %s", msg.value); 

		exit(EXIT_SUCCESS);
    }
}

void awaitPlayer() {
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

        printf("📭 waiting for message\n");
        msgrcv(msgid, &msg, sizeof(msg), 1, 0); 
        printf("📬 received: %s\n", msg.value);
    }
}

void sendMessage(int msg_type, char* msg_value) {
    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
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

int occupied[3] = { 0 };
int first_play = 0;
void waitForAllPlayers() {
    while (occupied[1] == 0 || occupied[2] == 0) {
		key_t key; 
		int msgid; 
		key = ftok("tictactoe", 65); 
		msgid = msgget(key, 0666 | IPC_CREAT); 
		msgrcv(msgid, &msg, sizeof(msg), server, 0); 

		msg.type = 4;
		if (occupied[atoi(msg.value)] == 0) {
			if (first_play == 0) first_play = atoi(msg.value);
			occupied[atoi(msg.value)] = 1;
			strncpy(msg.value, "100", 128);
			msgsnd(msgid, &msg, sizeof(msg), 0);
		} else if (occupied[atoi(msg.value)] == 1) {
			strncpy(msg.value, "200", 128);
			msgsnd(msgid, &msg, sizeof(msg), 0);
		} else {
			strncpy(msg.value, "400", 128);
			msgsnd(msgid, &msg, sizeof(msg), 0);
		}
    }

	sendMessage(x_player, "+");
	sendMessage(o_player, "+");
	sendMessage(x_player, "+");
	sendMessage(o_player, "+");
	sendMessage(x_player, "+");
	sendMessage(o_player, "+");
	sendMessage(x_player, "+");
	sendMessage(o_player, "+");
}

char* awaitMessage(int msg_type) {
	key_t key; 
	int msgid; 
	key = ftok("tictactoe", 65); 
	msgid = msgget(key, 0666 | IPC_CREAT); 

	msg.type = msg_type;
	msgrcv(msgid, &msg, sizeof(msg), msg_type, 0); 
	msgctl(msgid, IPC_RMID, NULL); 
	return msg.value;
}

char symbol[3] = { '0', 'X', 'O' };

int fill_count = 0;

int main() {
	clearTerminal();

    pid_t this_p = fork();
    if (this_p < 0) {
        throwError();
    } else if (this_p > 0) {
        int this_p_status;
        waitpid(this_p, &this_p_status, 0);
    } else {
		waitForAllPlayers();
		exit(EXIT_SUCCESS);
	}
	printf("all players logged in, starting game session\n");

	for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            board[i][j] = ' ';
        }
    }
	
    for (int i = 3 - first_play; true; ++i) {
		int cur_player = ((i % 2) * -1) + 2;
		int next_player = ((cur_player - 1) * -1) + 2;

		char* req = awaitMessage(server);
		printf("new message!\n");
		printf("    from: %c\n", symbol[cur_player]);
		printf("    msg : %s\n", req);
		printf("\n");

		int request = atoi(req);
		if (request < 1 || request > 9) { 
			char response[128];
			snprintf(response, sizeof(response), "%i", mark_badnum);
			sendMessage(cur_player, response);
		} 
		
		int row = (request - 1) / 3;
		int col = (request - 1) % 3;
		if (board[row][col] == ' ') { 
			board[row][col] = symbol[cur_player]; 

			// showBoard(board); 
			char response[128];
			snprintf(response, sizeof(response), "%i", request);
			sendMessage(cur_player, response);
			sendMessage(next_player, response);

			if (gameOver()) {
				char report[128];
				snprintf(report, sizeof(report), "%i", cur_player * 100);
				sendMessage(cur_player, report);
				sendMessage(next_player, report);
			} 
		} else if ((board[row][col] == symbol[cur_player])) { 
			char response[128];
			snprintf(response, sizeof(response), "%i", mark_selffill);
			sendMessage(cur_player, response);
			--i;
		} else {
			char response[128];
			snprintf(response, sizeof(response), "%i", mark_oppfill);
			sendMessage(cur_player, response);
			--i;
		}
	}
 
    return 0; 
}
