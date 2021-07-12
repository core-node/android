#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <math.h>

#define MAX 4096
#define TEXT 1024000
#define PORT 7337
#define SA struct sockaddr 

char url[MAX]; 
char httpok[]= "HTTP/1.1 200 OK\nServer: Dyper v0.1\nContent-Type: text/html\n\n";
char cmd[]="cat .";
char buffer[TEXT];
char in_buffer[TEXT];
char *out_buffer;
int test();
void func(int sockfd) 
{ 
        bzero(url, MAX); 
        read(sockfd, url, sizeof(url)); 
        bzero(buffer,TEXT);
		
		strcpy(in_buffer,cmd);
		strcat(in_buffer,"/index.html");
		FILE *fp = popen(in_buffer, "r");
		out_buffer=buffer;
		while(fgets(in_buffer,sizeof(in_buffer),fp)){
		   strcpy(out_buffer,in_buffer);
		   out_buffer+=strlen(out_buffer);
		   }
  		test();
		write(sockfd, httpok, sizeof(httpok));
		write(sockfd, buffer, sizeof(buffer));
		
} 

int serve() 
{ 

	
    int s, c, len; 
    struct sockaddr_in servaddr, cli; 

    s = socket(AF_INET, SOCK_STREAM, 0); 
    bzero(&servaddr, sizeof(servaddr)); 

  
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 

 
    if ((bind(s, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		return 1;
    }
    if ((listen(s, 5)) != 0) { 
        return 1;
    } 

    len = sizeof(cli); 
    
c = accept(s, (SA*)&cli, &len); 
    
	if (c < 0) { return 1; } 

    func(c);
    close(s);
    return 0;

} 











// ****
// writer
// ****

typedef struct {
int power;
int bits;
int start;
char *data;
} bitWriter;

void writeBit(bitWriter *p,int v){
  p->bits+=(v&1)<<(8-(++p->power));
  if(p->power>=8)
  {
     p->data[p->start++]=p->bits;
     p->power=0;
     p->bits=0;
  }
}

void writeValue(bitWriter *p, int v, int n){
    while(n>0)
    {
        n--;
        writeBit(p,(v>>n)&1);
    }
}

void flushWrite(bitWriter *p)
{
	while(p->power>0)
		writeBit(p, 0);
}

// ****
// reader
// ****
typedef struct{
int power;
int bits;
int start;
char *data;
}bitReader;

int readBit(bitReader *r){
    int v = (r->bits>>(8-(++r->power))&1);
    if(r->power>=8)
    {
        r->power=0;
        r->bits=r->data[r->start++];
    }
    return v;
}

int readValue(bitReader *r, int n){
    int v=0;
    while(n>0)
    {
       n--;
       v=v<<1;
       v+=readBit(r);
    }
    return v;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
 
/* -------- aux stuff ---------- */
void* mem_alloc(size_t item_size, size_t n_item)
{
  size_t *x = (size_t*)calloc(1, sizeof(size_t)*2 + n_item * item_size);
  x[0] = item_size;
  x[1] = n_item;
  return x + 2;
}
 
void* mem_extend(void *m, size_t new_n)
{
  size_t *x = (size_t*)m - 2;
  x = (size_t*)realloc(x, sizeof(size_t) * 2 + *x * new_n);
  if (new_n > x[1])
    memset((char*)(x + 2) + x[0] * x[1], 0, x[0] * (new_n - x[1]));
  x[1] = new_n;
  return x + 2;
}
 
void _clear(void *m)
{
  size_t *x = (size_t*)m - 2;
  memset(m, 0, x[0] * x[1]);
}
 
#define _new(type, n) mem_alloc(sizeof(type), n)
#define _del(m)   { free((size_t*)(m) - 2); m = 0; }
#define _len(m)   *((size_t*)m - 1)
#define _setsize(m, n)  m = (byte*)mem_extend(m, n)
#define _extend(m)  m = (byte*)mem_extend(m, _len(m) * 2)
 
 
/* ----------- LZW stuff -------------- */
typedef uint8_t byte;
typedef uint16_t ushort;

 bitWriter *newWriter(byte *out){
  bitWriter *r=(bitWriter*)_new(bitWriter,1);
  r->power=0;
  r->bits=0;
  r->start=0;
  r->data=(char*)out;
  return r;
}

bitReader *setReader(byte *in){
  bitReader *r =(bitReader*)_new(bitReader,1);
  r->power=0;
  r->bits=in[0];
  r->start=1;
  r->data=(char*)in;
  return r;
}
#define M_CLR 256 /* clear table marker */
#define M_EOD 257 /* end-of-data marker */
#define M_NEW 258 /* new code index */
 
/* encode and decode dictionary structures.
   for encoding, entry at code index is a list of indices that follow current one,
   i.e. if code 97 is 'a', code 387 is 'ab', and code 1022 is 'abc',
   then dict[97].next['b'] = 387, dict[387].next['c'] = 1022, etc. */
typedef struct {
  ushort next[256];
} lzw_enc_t;
 

typedef struct {
  ushort prev, back;
  byte c;
} lzw_dec_t;

byte *out,*in;
  int bits,out_len = 0, o_bits = 0;
  uint32_t tmp = 0;

 void write_bits(ushort x) {
    tmp = (tmp << bits) | x;
    o_bits += bits;
    if (_len(out) <= out_len) _extend(out);
    while (o_bits >= 8) {
      o_bits -= 8;
      out[out_len++] = tmp >> o_bits;
      tmp &= (1 << o_bits) - 1;
    }
}

byte* lzw_encode(byte *in, int max_bits)
{
  int len = _len(in), next_shift = 512;
  ushort code, c, nc, next_code = M_NEW;
  lzw_enc_t *d = (lzw_enc_t*)_new(lzw_enc_t, 4096);
  bits=9;
  
  byte *stop=in+len;
 
  out = (byte*)_new(ushort, 4);
  out_len = 0;
  o_bits = 0;
  tmp = 0;
  byte *e=(byte*)(void*)d;
  bitReader *br =setReader(in);
  
 
  //write_bits(M_CLR);
  for (code = *(in)++; in<stop; ) {
    c = *(in)++;
    if ((nc = d[code].next[c]))
      code = nc;
    else {
      write_bits(code);
      nc = d[code].next[c] = next_code++;
      code = c;
    }
 
    /* next new code would be too long for current table */
    if (next_code == next_shift) {
      /* either reset table back to 9 bits */
      if (++bits > max_bits) {
        /* table clear marker must occur before bit reset */
        write_bits(M_CLR);
 
        bits = 9;
        next_shift = 512;
        next_code = M_NEW;
        _clear(e);
      } else  /* or extend table */
        _setsize(e, next_shift *= 2);
		d=(lzw_enc_t*)e;
    }
  }
 
  write_bits(code);
  write_bits(M_EOD);
  if (tmp) write_bits(tmp);
 
  _del(d);
 
  _setsize(out, out_len);
  return out;
}

 void write_out(byte c)
  {
    while (out_len >= _len(out)) _extend(out);
    out[out_len++] = c;
  }

 byte* lzw_decode(byte *in)
{
  out = _new(byte, 4);
   out_len = 0;
 
  lzw_dec_t *d = _new(lzw_dec_t, 512);
  int len, j, next_shift = 512, bits = 9, n_bits = 0;
  ushort code, c, t, next_code = M_NEW;
 
  uint32_t tmp = 0;
  byte *e=(byte*)(void*)d;
  
 
  _clear(d);
    for (j = 0; j < 256; j++) d[j].c = j;
    next_code = M_NEW;
    next_shift = 512;
    bits = 9; 
	
  for (len = _len(in); len;) {
    while(n_bits < bits) {
      if (len > 0) {
        len --;
        tmp = (tmp << 8) | *(in++);
        n_bits += 8;
      } else {
        tmp = tmp << (bits - n_bits);
        n_bits = bits;
      }
    }
    n_bits -= bits;
    code = tmp >> n_bits;
    tmp &= (1 << n_bits) - 1;
    if (code == M_EOD) break;
    if (code == M_CLR) {
      _clear(d);
    for (j = 0; j < 256; j++) d[j].c = j;
    next_code = M_NEW;
    next_shift = 512;
    bits = 9;
      continue;
    }
 
    if (code >= next_code) {
      fprintf(stderr, "Bad sequence\n");
      _del(out);
      goto bail;
    }
 
    d[next_code].prev = c = code;
    while (c > 255) {
      t = d[c].prev; d[t].back = c; c = t;
    }
 
    d[next_code - 1].c = c;
 
    while (d[c].back) {
      write_out(d[c].c);
      t = d[c].back; d[c].back = 0; c = t;
    }
    write_out(d[c].c);
 
    if (++next_code >= next_shift) {
      if (++bits > 16) {
        /* if input was correct, we'd have hit M_CLR before this */
        fprintf(stderr, "Too many bits\n");
        _del(out);
        goto bail;
      }
      _setsize(e, next_shift *= 2);
	  d=(lzw_dec_t*)e;
    }
  }
 
  /* might be ok, so just whine, don't be drastic */
  if (code != M_EOD) fputs("Bits did not end in EOD\n", stderr);
 
  _setsize(out, out_len);
bail: _del(d);
  return out;
}


typedef struct {
    void **data;
	int len;
	int cnt;
	void *parent; }xe;
void setDAT(int num, char *value, xe*k);
char *getDAT(int nk, xe *k);
void setDAV(int num, char *value, xe*k);
char *getDAV(int nk, xe *k);
xe *sl;int vxx;
void addx(void *ptr, void *tr){xe *xx=(xe*)ptr;setDAV(xx->cnt++,(char*)tr,xx);}
void addxe(void* ptr,char *text){xe *xx = (xe*)ptr;setDAV(xx->cnt++,text,xx);char *t = text;
   while (*t!=0)t+=2;xx->len += (t-text);}
char zero[2]= "0";char nptr[255];
char * intToStr(int value){ float v=value;int t=0;int full=0;
  char *text=malloc(1024);while(v>=100)v=v/100.0;
while(value>full){
text[t++]=(int)(v/10)+'0';
int u=v-(int)(v/10)*10;
text[t++]=u+'0';
full=full*100+(int)v;v=(v-(int)v)*100.01;}
 return text;}

void addxi(void* ptr, int value){xe* xx = (xe*)ptr;char *text;int sze;
  if (value==0){text = zero;sze=1;}else text=intToStr(value);
  setDAV(xx->cnt++,text,xx);xx->len += strlen(text);}
void addxv(void *ptr, int value){xe* xx = (xe*)ptr;vxx = value;void *vPtr = &vxx;char *ok = (char*)malloc(5);
   strcpy(ok, (char*)vPtr);setDAV(xx->cnt++,ok,xx);xx->len += 4;}
char *join(xe* xxe, char *by){char* ret = (char*)malloc(xxe->len+2);char *s, *d = ret;int cn=0;
   int b = (*by) ? strlen(by) : 0; char *bs;
   while(cn<xxe->cnt){s=getDAV(cn++,xxe);while(*s!=0)strncpy(d++,s++,1);
   if (b){bs = by; strncpy(d,bs,b);d+=b;}}strncpy(d,"\0",1);
	return ret;}

char empty[]="\0";
//lada 2x2
#define cr 2
#define crc(sn) ((int)(*sn) ^ ((*(sn+1))^0x3C))
#define MIN_ALLOC 1024

#define nodeSize 256
#define nodeFill 255
#define key xe*
#define strCopySized(str,src,size) strncpy(str,src,size); 
#define strCopy(str,src) strCopySized(str, src, strlen(src)) 

#define c(a) addxe(&x, a);
#define v(a) addxv(&x, a);
#define lne "<br/>"
#define line c(lne);
#define space c(" ");
#define m(inte) addxi(&x, inte);

#define done  strcpy(out_buffer,join(&x,""));
// console ': 4.0 Mb  text-space
#define xinit  x.cnt = 0; x.len = 0; x.data = malloc(1048576*4); 
#define stm xe *k = malloc(sizeof(xe));xnew(k) 
//xnew 4096 per node : allows 512Gb Total
#define xnew(p)  p->cnt =0;p->len =0;p->data=malloc(1024);
#define name(n,v,k) setCRC(n,v,k);
#define set(n,v) name(n,v,k)
#define get(s) find(k, s)
#define push(s) sl=(xe*)malloc(sizeof(xe)); set(s,sl) xnew(sl) sl->parent=(void*)k; k=sl;
#define back if(k->parent){k =k->parent;}
#define select(s) k= (xe*)find(k,s);
#define _(s) c(get(s))
#define end done return 0; }

void setCRC(char *name, void *value, xe*k){if(strlen(name)>cr){xe *xk = malloc(sizeof(xe)); xnew(xk) k->data[crc(name)]=xk; setCRC(name+cr,value,xk);} else k->data[crc(name)]=value;}
char *find(xe *k, char *n){ if(strlen(n)>cr){xe *xk = (xe*)k->data[crc(n)]; return find(xk,n+cr);}else return k->data[crc(n)];}
void setDAT(int num, char *value, xe*k){char *name =intToStr(num);setCRC(name, value,k);}
char *getDAT(int nk, xe *k){char *n=intToStr(nk);return find(k, n);}
void setDAV(int num, char *value, xe*k){ k->data[num]=value;}
char *getDAV(int nk, xe *k){return k->data[nk];}

char *before(char *str, char *src){ char *s = str;int b = (*src) ? strlen(src) : 0; char *ret=(char*)malloc(strlen(str)+1);	char *d=ret;   while(*s!=0){if (b){ char *v=src; char *t=s;while(*t==*v){t++; v++;if ((v-src)==b){ if (d==(ret+b-1)) return empty; strCopySized(d,empty,1); return ret;}}}strCopySized(d++,s++,1);}   return str;}
char *after(char *str, char *src){ char *s = str;int b = (*src) ? strlen(src) : 0;	   while(*s!=0){if (b){ char *d=src; while(*s==*d){s++; d++; if ((d-src)==b) return s;}}s++;}   return str;}

key split(char* src, char* sep, key xk){ char* in=malloc(strlen(src)+1);	strcpy(in, src);xnew(xk) char* e=after(in, sep);	 addxe(xk, before(in,sep)); while(e!=in){*(e-1)=0;  in=e; addxe(xk, before(e,sep)); e=after(in, sep); }	return xk;}
char* replace(char* in, char* sep, char* by){key xk = split(in, sep, (key)malloc(sizeof(xe))); return join(xk, by);}

#define INTEGER 254
#define STRINGS 253
#define NUMBERS 252
#define RUNABLE 251

typedef struct {
  xe *root;
  xe *node;

  char id[1024];
  char op[1024];
  char nb[1024];

	int len_id;
	int len_op;
	int len_nb;
        int sym;
        int len;

 char *text;
 char here;

} scripter;
scripter sys;

void addNumber()
{
   char hexa[]="0123456789ABCDEF";
   char oct[]="01234567";
   char bin[]="01";

   sys.nb[sys.len_nb]=0;

   xe *value = (xe*) malloc(sizeof(xe));
   value->parent = (void*) NUMBERS;
   double *data=calloc(1,sizeof(double));
   *data=atof(sys.nb);
   value->data = (void**) data;

   addx(sys.node,value);

   //todo : check hexa/oct/bin integer value
   //todo : check 'e' token
   //todo : optimize to integer when possible
}

void addOperator()
{
   char *order[]={".","++","--","~","!","**","*","/","%","+","-","<<",">>","<<<",">>>","<",">","<=",">=","==","!=","===","!==","&","^","|","&&","||","=","+=","-=","*=","/=","**=","%=","<<=",">>=","<<<=",">>>=","&=","^=","|="};

   sys.op[sys.len_op]=0;

}

void addIdentifier()
{
   sys.id[sys.len_id]=0;

}

typedef xe *(*token)();
token tokens[256];

void addToken()
{
    xe *node=tokens[sys.here]();
    addx(sys.node,node);
}

void script_next();
xe *script(char *text)
{
  sys.text=text;
  sys.sym=0;
  sys.len=strlen(text);
  while(sys.sym<sys.len)script_next();
}
   char brli[]=" \n\r\t";
   char oper[]="~^|&!=<>+-*/%.";
   char tokn[]="`'\"(){}[]?:;,";
   char nmbr[]="0123456789-";

void script_next()
{
  while(sys.sym<sys.len)
  {
	sys.here=sys.text[sys.sym++];

	if (sys.len_id==0 && strchr(nmbr,sys.here)>0){
         sys.nb[sys.len_nb++]=sys.here;
	 if(sys.len_op>0){addOperator();sys.len_op=0;return;}
       }else if(strchr(oper,sys.here)>0){
	 sys.op[sys.len_op++]=sys.here;
	 if(sys.len_id>0){addIdentifier();sys.len_id=0;return;}
	 if(sys.len_nb>0){addNumber();sys.len_nb=0;return;}
        }else if(strchr(tokn,sys.here)>0){
	 if(sys.len_op>0){addOperator();sys.len_op=0;return;}
	 if(sys.len_nb>0){addNumber();sys.len_nb=0;return;}
	 if(sys.len_id>0){addIdentifier();sys.len_id=0;return;}
	 addToken();
        }else if(strchr(brli,sys.here)>0){
	 if(sys.len_op>0){addOperator();sys.len_op=0;return;}
	 if(sys.len_nb>0){addNumber();sys.len_nb=0;return;}
	 if(sys.len_id>0){addIdentifier();sys.len_id=0;return;}
       }else{
	 if(sys.len_op>0){addOperator();sys.len_op=0;return;}
	 if(sys.len_nb>0){addNumber();sys.len_nb=0;return;}
	 sys.id[sys.len_id++]=sys.here;
      }
  }

}

xe *modules(xe *root)
{
}


xe *run(xe *rootscript)
{
}

byte*enc;
bitWriter *zw;
xe x;



int test()
{
   xinit; stm
   line
   int i,fd = open("uiux.html", O_RDONLY);
 
  if (fd == -1) {
    c( "Can't read file");
	done
    
  };
 
  struct stat st;
  fstat(fd, &st);
 
  byte *in = (byte*)_new(char, st.st_size);
  read(fd, in, st.st_size);
  _setsize(in, st.st_size);
  close(fd);
 
  c("input size: ") m(_len(in)) line
  
 


  
int left;
int right;
void *next;
  
void *add=&&plus;
void *sub=&&minus;
	next=&&zero;
	left=0;
	
  goto *next;
  
  plus:
  left = left + right;
  goto *next;
  
  minus:
  left = left - right;
  goto *next;
  
  zero:
  left = 0;
  
  /*** our vm goes here ***/
  /*** todo : benchmark from standard c and nodejs ***/
  /*** 7100 + 237 - 340 + 23 - 39 ***/
  
  next=&&a;
  left=7100;
  right=237;
  goto *add;
  
  a:
  m(left) line
  next=&&b;
  right=340;
  goto *sub;
  
  
  b:
  m(left) line
  next=&&c;
  right=23;
  goto *add;
  
  c:
  m(left) line
  next=&&d;
  right=39;
  goto *sub;
  
  d:
  m(left) line

   done
  
}


int main(int argc, char **argv)
{
	serve();
}
