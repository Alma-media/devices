#pragma pack(push, 1)
class Substring {
  public:
    Substring(int start, int end);
    
    ~Substring();
    
    char* extract(char* src);

  private:
    char* buffer;
    int   start;
    int   total;
};
#pragma pack(pop)
