#include "asio.hpp"
#include "ChatMessage.hpp"
#include <stdio.h>
#include <string.h>


int main() {

	ChatMessage msg{ "Keum", "Good Job" };
	std::cout << msg.serialize() << "\n";
	auto str = msg.serialize();
	ChatMessage msg2;
	msg2.deSerialize(str);
	std::cout << msg2.serialize() << "\n";

	return 0;
}