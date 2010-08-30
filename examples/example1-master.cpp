#include <SerPro-glib.h>
#include <pty.h>

SERPRO_GLIB_BEGIN();

IMPORT_FUNCTION(1, function1, void (int) );
IMPORT_FUNCTION(2, multiply, int (int,int) );

void connect() {
	int m;
	std::cerr<<"["<<getpid()<<"] MASTER: calling function1"<<std::endl;
	function1( 10 );
	std::cerr<<"["<<getpid()<<"] MASTER: calling multiply 13x20"<<std::endl;
	m = multiply(13,20);
	std::cerr<<"["<<getpid()<<"] MASTER: multiply 13x20 == "<<m<<std::endl;
}

SERPRO_GLIB_END();

int main()
{
	int master,slave;
	char device[PATH_MAX];

	if (openpty(&master,&slave,device,NULL,NULL)<0) {
		perror("openpty");
		return -1;
	}
	fprintf(stderr,"Opened PTY %s\n", device);

	if (SerProGLIB::init(master)<0)
		return -1;

	switch (fork()) {
	case 0:
		return execl("./example1-slave","example1-slave", device, NULL);
	default:
		break;
	}

	SerProGLIB::onConnect( &connect );
	SerProGLIB::start();
	SerProGLIB::run();
}
