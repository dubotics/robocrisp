#include <cstdio>
#include "Buffer.hh"

#include "DataStructure.hh"
#include "SArray.hh"
using namespace Robot;



static void
print_api_comment(const APIElement& ae, FILE* fp, bool after = false)
{
  fprintf(fp, "/**%s %s%s %s */\n", after ? "<" : "", ae.use, ae.use ? (ae.use[strlen(ae.use)-1] == '.' ? "" : "." ) : "", ae.description);
}

template  < typename _T >
static void
dump_field(const DataField<_T>& field)
{
  fprintf(stderr, "  %s %s; \t", field.type_name(), field.symbol);
  print_api_comment(static_cast<const APIElement&>(field), stderr, true);
}

static void
dump_structure(const DataStructure& s, TargetLanguage lang = TargetLanguage::CXX)
{
  print_api_comment(s, stderr);

  if ( lang == TargetLanguage::C )
    fprintf(stderr, "typedef struct\n{\n");
  else
    fprintf(stderr, "struct %s\n{\n", s.symbol);

  for ( const DataField<>& field : s.fields )
    dump_field(field);

  if ( lang == TargetLanguage::C )
    fprintf(stderr, "} %s;\n", s.symbol);    
  else
    fprintf(stderr, "};\n");
}

int
main(int argc, char* argv[])
{
  using namespace Robot::keywords;
  
  DataStructure my_data ( "MyData", "Tests the data structure code", "This documentation sure is dumb, isn't it?",
			  {   DataField<uint8_t>( "integer", "Tests integer field type", "This is just a test integer, Joe.", _neutral = 0, _minimum = -1, _maximum = 100),
			      DataField<float>( "some_float", "Tests floating-point field type", "Let's call him \"Bob\".") } );
  dump_structure(my_data);

      
  return 0;
}
