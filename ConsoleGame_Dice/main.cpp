#include <iostream>
#include <string>
#include "player.h"

using namespace std;

/*
int main() {

	cout << "It is a biplayer game\n"
		<< "Please create two players\n";
	
	string player_1, player_2;
	cout << "Please enter name for player 1: ";
	cin >> player_1;
	cout << "Please enter name for player 2: ";
	cin >> player_2;

	player A(player_1, 0);
	player B(player_2, 1);

	while (!A.victory && !B.victory) {
		if (A.current) {
			A.run();
			B.current = true;
		}
		else if (B.current) {
			B.run();
			A.current = true;
		}
	}



	system("pause");

	return 0;
}
*/


/*
The sequence of the game should be specified explicitly
maybe in the generic sequence with which they were created
*/


bool is_victory(vector<player>& players) {

	bool victory = false;
	for (auto player : players) {
		victory |= player.victory;
	}
	cout << "Any victory? " << boolalpha << victory << endl;
	return victory;

}


int main() {

	int player_num;
	string player_name;
	vector<player> players;

	cout << "It is a multiplayer game\n"
		<< "How many of you want to join in? ";
	cin >> player_num;
	cout << endl;


	for (int i = 0; i < player_num; i++) {
		cout << "Please enter No" << i << " player name ";
		cin >> player_name;
		player A(player_name, i);
		players.push_back(A);
	}

	while (!is_victory(players)) {
		/*for (auto player : players) {
			if (player.current) {
				player.run();
				players = next_player(players, player);
				break;
			}
		}*/

		for (int i = 0; i < players.size(); i++) {
			if (players[i].current) {
				players[i].run();
				players[i].current = false;
				if (i != players.size() - 1) 
					players[++i].current = true;
				else
					players[0].current = true;

				break;
			}
		}
	}


	system("pause");

	return 0;
}