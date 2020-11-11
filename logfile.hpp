#include <iostream>
#include <map>
#include <memory>
#include <variant>

#include "ConstBitStream.hpp"

struct node_t;

typedef std::variant<
  std::uint8_t,
  std::int8_t,
  std::uint16_t,
  std::int16_t,
  std::uint32_t,
  std::int32_t,
  float,
  std::string,
  std::vector<std::uint8_t>,
  std::vector<std::int8_t>,
  std::vector<std::uint16_t>,
  std::vector<std::int16_t>,
  std::vector<std::uint32_t>,
  std::vector<std::int32_t>,
  std::vector<float>,
  std::vector<std::string>,
  node_t,
  std::vector<node_t>
> value_t;

struct node_t : public std::map<std::string, value_t> {};

struct type_definition_t
{
  size_t type_code;
  size_t size;
  bool is_signed_type;
  size_t pointer_code;
  size_t storage_code;
  size_t array_size;
  std::shared_ptr<type_definition_t> array_type_definition;
  std::vector<std::pair<std::string, std::shared_ptr<type_definition_t>>> record_type_definitions;
};

typedef std::map<size_t, type_definition_t> type_definitions_t;

struct variable_definition_t
{
  std::string name;
  std::size_t type_code;
  std::uint8_t mtype;
  std::uint32_t adr;
  std::uint16_t attrib;
};

typedef std::vector<variable_definition_t> variable_definitions_t;

struct xrt_t
{
  std::uint8_t version;
  type_definitions_t type_definitions;
  variable_definitions_t variable_definitions;
  std::uint16_t nbytes;
  std::uint8_t fattrib;
};

node_t
read_header(ConstBitStream& stream)
{
  node_t header;
  header["text"] = stream.read_string();
  header["mbc_id"] = stream.read_string();
  header["serial_number"] = stream.read_string();
  header["server_version"] = stream.read_uint16le();
  header["format_version"] = stream.read_uint8();
  header["timestamp_year"] = stream.read_uint16le();
  header["timestamp_month"] = stream.read_uint8();
  header["timestamp_day"] = stream.read_uint8();
  header["timestamp_hour"] = stream.read_uint8();
  header["timestamp_minute"] = stream.read_uint8();
  header["timestamp_second"] = stream.read_uint8();
  header["fault_counter"] = stream.read_int32le(); 
  header["supervision_state"] = stream.read_uint8_vector(stream.read_uint32le());
  stream.read_uint32le(); // supervision crc
  stream.read_uint32le(); // crc
  return header;
}

type_definition_t
read_type_definition(ConstBitStream& stream, size_t type_code, type_definitions_t& type_definitions)
{
  type_definition_t type_definition;
  type_definition.type_code = type_code;
  if (type_code == 4)
    {
      auto value = stream.read_uint8();
      type_definition.size =  value & 127;
      type_definition.is_signed_type = ((value & 128) == 1) ? true : false;
    }
  else if (type_code == 5)
    {
      type_definition.pointer_code = stream.read_uint16le();
      type_definition.storage_code = stream.read_uint8();
      type_definition.size = 32;
    }
  else if (type_code == 6)
    {
      auto array_code = stream.read_uint16le();
      type_definition.array_type_definition = std::make_shared<type_definition_t>(read_type_definition(stream, array_code, type_definitions));
      type_definition.array_size = stream.read_uint32le();
    }
  else if (type_code == 7)
    {
      auto record_size = stream.read_uint8();
      std::vector<std::pair<std::string, std::shared_ptr<type_definition_t>>> record_type_definitions;
      for (size_t n = 0; n < record_size; n++)
        {
          auto record_name = stream.read_string();
          auto record_code = stream.read_uint16le();
          auto record_type_definition = read_type_definition(stream, record_code, type_definitions);
          type_definition.record_type_definitions.push_back(std::make_pair(record_name, std::make_shared<type_definition_t>(record_type_definition)));
        }
    }

  return type_definition;
}

type_definitions_t
read_type_definitions(ConstBitStream& stream)
{
  type_definitions_t type_definitions;
  while (stream.good())
    {
      auto base_code = stream.read_uint8();
      if (base_code == 0)
        break;
      auto user_code = stream.read_uint16le();
      type_definitions[user_code] = read_type_definition(stream, base_code, type_definitions);
    }
  return type_definitions;
}

variable_definitions_t
read_variable_definitions(ConstBitStream& stream)
{
  variable_definitions_t variable_definitions;
  while (stream.good())
    {
      auto name = stream.read_string();
      if (name.compare("\x00") == 0)
        break;
      variable_definition_t variable_definition;
      variable_definition.name = name;
      variable_definition.type_code = stream.read_uint16le();
      variable_definition.mtype = stream.read_uint8();
      variable_definition.adr = stream.read_uint32le();
      variable_definition.attrib = stream.read_uint16le();
      variable_definitions.push_back(variable_definition);
    }
  return variable_definitions;
}

xrt_t
read_xrt(ConstBitStream& stream)
{
  xrt_t xrt;
  stream.read_uint32le(); // number of bytes xrt
  xrt.version = stream.read_uint8();
  xrt.type_definitions = read_type_definitions(stream);
  xrt.variable_definitions = read_variable_definitions(stream);
  if (xrt.version >= 128)
    {
      xrt.nbytes = stream.read_uint16le();
      xrt.fattrib = stream.read_uint8();
    }
  else
    {
      xrt.nbytes = 0;
      xrt.fattrib = 0;
    }

  stream.read_uint32be(); // crc

  return xrt;
}

value_t
read_by_base_type_code(ConstBitStream& stream, std::uint16_t base_type_code)
{
  if (base_type_code == 0)
    return stream.read_int32le();
  if (base_type_code == 1)
    return stream.read_uint32le();
  if (base_type_code == 2)
    return stream.read_float32le();
  if (base_type_code == 3)
    return stream.read_uint8();
  if (base_type_code == 8)
    return stream.read_int16le();
  if (base_type_code == 9)
    return stream.read_uint16le();
  throw std::runtime_error("Wrong base type code");
}

void
read_by_type_definition(ConstBitStream& stream, node_t& node, const type_definition_t& type_definition, type_definitions_t& type_definitions, const std::string& parent)
{
  auto type_code = type_definition.type_code;
  if (type_code == 0 or type_code == 1 or type_code == 2 or type_code == 3 or type_code == 8 or type_code == 9)
    node[parent] = read_by_base_type_code(stream, type_code);
  else if (type_code == 4)
    node[parent] = stream.read_bitsintle(type_definition.size, type_definition.is_signed_type);
  else if (type_code == 6)
    {
      auto& array_type_definition = type_definition.array_type_definition;
      for (size_t n = 0; n < type_definition.array_size; n ++)
        read_by_type_definition(stream, node, *array_type_definition, type_definitions, parent + "[" + std::to_string(n) + "]");
    }
  else if (type_code == 7)
    {
      for (auto& [record_name, record_type_definition] : type_definition.record_type_definitions)
        {
          read_by_type_definition(stream, node, *record_type_definition, type_definitions, parent + "." + record_name);
        }
    }
  else 
   read_by_type_definition(stream, node, type_definitions.at(type_code), type_definitions, parent);
}

node_t
read_variables(ConstBitStream& stream, xrt_t& xrt)
{
  node_t variables;
  auto current_pos = stream.tellg(); 
  for (auto& variable_definition : xrt.variable_definitions)
    {
      stream.seekg(variable_definition.adr + current_pos); // advance to address
      auto& type_definition = xrt.type_definitions[variable_definition.type_code];
      read_by_type_definition(stream, variables, type_definition, xrt.type_definitions, variable_definition.name);
    }
  stream.seekg(xrt.nbytes + current_pos);
  return variables;
}

node_t
read_supervision(ConstBitStream& stream)
{
  node_t supervision;
  auto length_supervision = stream.read_uint32le() - 4;
  auto offset_supervision = stream.tellg();
  supervision["file_version"] = stream.read_uint8();
  supervision["file_attribute"] = stream.read_uint8();
  supervision["faultcode"] = stream.read_uint16le_vector(stream.read_uint16le());
  auto length_faultnames = stream.read_uint32le();
  std::vector<std::string> faultnames;
  auto offset = stream.tellg();
  while (stream.tellg() - offset < length_faultnames)
    faultnames.push_back(stream.read_string());
  supervision["faultnames"] = faultnames;
  auto length_faulthelp = stream.read_uint32le();
  std::vector<std::string> faulthelp;
  offset = stream.tellg();
  while (stream.tellg() - offset < length_faulthelp)
    faulthelp.push_back(stream.read_string());
  supervision["faulthelp"] = faulthelp;
  if (stream.tellg() - offset_supervision < length_supervision)
    supervision["faultmask"] = stream.read_uint32le_vector(stream.read_uint32le());
  stream.read_uint32le(); // crc
  return supervision;
}

node_t
read_pack0(ConstBitStream& stream)
{
  auto xrt = read_xrt(stream);
  return read_variables(stream, xrt);
}

node_t
read_pack1(ConstBitStream& stream)
{
  node_t pack;
  pack["sampling_rate"] = stream.read_uint32le();
  pack["pre_trigger_length"] = stream.read_uint32le();
  auto xrt = read_xrt(stream);
  auto length_data = stream.read_int32le();
  std::vector<node_t> data;
  if (length_data == -1)
    {
      while (stream.good())
        data.push_back(read_variables(stream, xrt));
    }
  else
    { 
      for (size_t n = 0; n < static_cast<size_t>(length_data); n++)
        data.push_back(read_variables(stream, xrt));
    } 
  pack["data"] = data;
  return pack;
}

node_t
read_pack7(ConstBitStream& stream)
{
  auto states = stream.read_uint8_vector(stream.read_uint32le());
  auto supervision = read_supervision(stream);
  supervision["states"] = states;
  return supervision;
}

std::vector<node_t>
read_body(ConstBitStream& stream)
{
  std::vector<node_t> body;
  while (stream.good())
    {
      auto pack_type = stream.read_uint8();
      //std::cout << "Reading pack type: " << (int)pack_type << std::endl;

      if (pack_type == 0)
        body.push_back(read_pack0(stream));
      else if (pack_type == 1)
        body.push_back(read_pack1(stream));
      else if (pack_type == 7)
        body.push_back(read_pack7(stream));
      else
        throw std::runtime_error(std::string("Unknown pack type ") + std::to_string(pack_type));

      body.back()["type"] = pack_type;

      if (pack_type != 8 and stream.length() - stream.tellg() >= 4)
        stream.read_uint32le();
    }
  return body;
}

node_t
read_file_header(const std::string& filename)
{
  ConstBitStream stream(filename);
  return read_header(stream);
}

std::tuple<node_t, std::vector<node_t>>
read_file(const std::string& filename)
{
  ConstBitStream stream(filename);
  auto header = read_header(stream);
  auto body = read_body(stream);
  return std::make_tuple(header, body);
}
