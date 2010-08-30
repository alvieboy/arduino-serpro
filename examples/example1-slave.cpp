#include <SerPro-glib.h>

SERPRO_GLIB_SLAVE_BEGIN();

void function1(int a)
{
	std::cerr<<"["<<getpid()<<"] SLAVE: Called "<<__FUNCTION__<<" with "<<a<<std::endl;
}

int multiply(int a, int b)
{
	std::cerr<<"["<<getpid()<<"] SLAVE: "<<__FUNCTION__<<" asked to multiply "<<a<<" and "<<b<<std::endl;
	return a * b;
}

EXPORT_FUNCTION(1, function1);
EXPORT_FUNCTION(2, multiply);

SERPRO_GLIB_END();

int main(int argc, char **argv)
{

	if (argc<2)
		return -1;

	if (SerProGLIB::init(argv[1])<0)
		return -1;

	SerProGLIB::run();
}
