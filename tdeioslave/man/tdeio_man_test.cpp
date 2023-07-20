

#include <tqobject.h>

#include "tdeio_man.h"


#include <tdeapplication.h>
#include <tdelocale.h>


class tdeio_man_test : public  MANProtocol
{
  TQ_OBJECT

public:
  tdeio_man_test(const TQCString &pool_socket, const TQCString &app_socket);

protected:
  virtual void data(int);

};





int main(int argc, char **argv)
{
  TDEApplication a( argc, argv , "p2");

  MANProtocol testproto("/tmp/tdeiotest.in", "/tmp/tdeiotest.out");
  testproto.showIndex("3");

  return 0;
}


