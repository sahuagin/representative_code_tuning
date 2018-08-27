static const std::string IN{"INPUT/"};
static const std::string IN_HASH{ IN + "hashid/"};
static const std::string IN_KEY{ IN + "keyid/"};
static const std::string IN_DATA{ IN + "data/"};

static const std::string HT{"HT/"};
static const std::string HT_TABLE{HT + "/table"};
static const std::string HT_DATA{HT + "/data"};

static const std::string TAB_TEMP{"%s/tab.XXXXXX"};
static const std::string DAT_TEMP{"%s/dat.XXXXXX"};

static const std::string IN_FILE_KEY{"%s/" + IN_KEY + "file_key.%03d"};
static const std::string IN_FILE_HASH{"%s/" + IN_HASH + "file_hash.%03d"};
static const std::string IN_FILE_LEN{"%s/" + IN_HASH + "file_length.%03d"};
static const std::string IN_FILE_DATA{"%s/" + IN_DATA + "file_data_runon.%03d"};


#pragma pack(push, 1)
struct rec {
  std::uint64_t hash;
  std::uint16_t len;
};
#pragma pack(pop)

