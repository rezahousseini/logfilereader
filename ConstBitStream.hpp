#include <fstream>
#include <vector>
#include <cmath>
#include <cassert>

std::string iso_8859_1_to_utf8(const std::string& in)
{
  std::string out;
  for (std::uint8_t c : in)
    {
      if (c < 0x80)
        out.push_back(c);
      else
        {
          out.push_back(0xc0 | c >> 6);
          out.push_back(0x80 | (c & 0x3f));
        }
    }
  return out;
}

class ConstBitStream
{
  public:

  ConstBitStream(const std::string& filename) : input_(filename, std::ios::binary), bit_pos_(0)
  {
    input_.seekg(0, input_.end);
    len_ = input_.tellg();
    input_.seekg(0, input_.beg);
  }

  private:

  std::ifstream input_;
  std::streamsize len_;
  size_t bit_pos_;

  public:

  inline std::string
  read_string()
  {
    std::vector<char> buffer;
    char c;
    while (input_.good())
      {
        input_.get(c);
        if (static_cast<std::uint8_t>(c) == 0)
          break;
        buffer.push_back(c);
      }
    return iso_8859_1_to_utf8(std::string(buffer.begin(), buffer.end()));
  }
  
  inline std::uint8_t
  read_uint8()
  {
    std::uint8_t value;
    input_.read(reinterpret_cast<char*>(&value), 1);
    return value;
  }
  
  inline std::int8_t
  read_int8()
  {
    std::int8_t value;
    input_.read(reinterpret_cast<char*>(&value), 1);
    return value;
  }
  
  inline std::uint16_t
  read_uint16le()
  {
    return static_cast<std::uint16_t>(read_uint8()) |
           static_cast<std::uint16_t>(read_uint8()) << 8;
  }
  
  inline std::int16_t
  read_int16le()
  {
    return static_cast<std::int16_t>(read_uint16le() ^ 0xffff);
  }
  
  inline std::uint32_t
  read_uint32le()
  {
    return static_cast<std::uint32_t>(read_uint8()) |
           static_cast<std::uint32_t>(read_uint8()) <<  8 |
           static_cast<std::uint32_t>(read_uint8()) << 16 |
           static_cast<std::uint32_t>(read_uint8()) << 24;
  }
  
  inline std::uint32_t
  read_uint32be()
  {
    return static_cast<std::uint32_t>(read_uint8()) << 24 |
           static_cast<std::uint32_t>(read_uint8()) << 16 |
           static_cast<std::uint32_t>(read_uint8()) <<  8 |
           static_cast<std::uint32_t>(read_uint8());
  }
  
  inline std::int32_t
  read_int32le()
  {
    return static_cast<std::int32_t>(read_uint32le());
  }
  
  inline float
  read_float32le()
  {
    float value;
    input_.read(reinterpret_cast<char*>(&value), 4);
    return value;
  }
  
  inline std::vector<std::uint8_t>
  read_uint8_vector(size_t num_elements)
  {
    std::vector<std::uint8_t> buffer;
    for (size_t n = 0; n < num_elements; n++)
      buffer.push_back(read_uint8());
    return buffer;
  }

  inline std::vector<std::uint16_t>
  read_uint16le_vector(size_t num_elements)
  {
    std::vector<std::uint16_t> buffer;
    for (size_t n = 0; n < num_elements; n++)
      buffer.push_back(read_uint16le());
    return buffer;
  }

  inline std::vector<std::uint32_t>
  read_uint32le_vector(size_t num_elements)
  {
    std::vector<std::uint32_t> buffer;
    for (size_t n = 0; n < num_elements; n++)
      buffer.push_back(read_uint32le());
    return buffer;
  }
  
  inline std::int32_t
  read_bitsintle(size_t size, bool is_signed_type)
  {
    std::uint32_t mask = (1 << size) - 1;
    std::uint32_t value = (read_uint32le() >> bit_pos_) & mask; 
    bit_pos_ = (bit_pos_ + size) & 7; // update bit position
    auto has_sign = 1 << (size - 1) == 1 ? true : false;
    if (is_signed_type and has_sign)
      return value | (mask ^ (-1));
    else  
      return value;
  }

  inline bool good() { return input_.good(); }

  inline std::streampos tellg() { return input_.tellg(); }

  inline std::istream& seekg(std::streampos pos) { return input_.seekg(pos); }

  inline std::streamsize length() const { return len_; }

  //inline std::streamsize length()
  //{
  //  auto current_pos = input_.tellg();
  //  input_.seekg(0, input_.end);
  //  auto len = input_.tellg();
  //  input_.seekg(current_pos);
  //  std::cout << len << " " << len_ << std::endl;
  //  //assert(len == len_);
  //  return len;
  //}
};
