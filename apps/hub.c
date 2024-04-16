#include "../src/server.h"

int main() {
	struct server server;
	create_server(&server);
	start_server(&server); 	
	return 0;
}
