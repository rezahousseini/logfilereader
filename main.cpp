#include "logfile.hpp"

int main(int argc, const char* argv[])
{
  if (argc != 2)
    {
      std::cerr << "Usage: logfilereader <filename>" << std::endl;
      return EXIT_FAILURE;
    }

  std::string filename(argv[1]);
  ConstBitStream stream(filename);

  auto header = read_header(stream);
  auto body = read_body(stream);

  return EXIT_SUCCESS;
}
