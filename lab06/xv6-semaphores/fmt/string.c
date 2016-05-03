6150 #include "types.h"
6151 #include "x86.h"
6152 
6153 void*
6154 memset(void *dst, int c, uint n)
6155 {
6156   if ((int)dst%4 == 0 && n%4 == 0){
6157     c &= 0xFF;
6158     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
6159   } else
6160     stosb(dst, c, n);
6161   return dst;
6162 }
6163 
6164 int
6165 memcmp(const void *v1, const void *v2, uint n)
6166 {
6167   const uchar *s1, *s2;
6168 
6169   s1 = v1;
6170   s2 = v2;
6171   while(n-- > 0){
6172     if(*s1 != *s2)
6173       return *s1 - *s2;
6174     s1++, s2++;
6175   }
6176 
6177   return 0;
6178 }
6179 
6180 void*
6181 memmove(void *dst, const void *src, uint n)
6182 {
6183   const char *s;
6184   char *d;
6185 
6186   s = src;
6187   d = dst;
6188   if(s < d && s + n > d){
6189     s += n;
6190     d += n;
6191     while(n-- > 0)
6192       *--d = *--s;
6193   } else
6194     while(n-- > 0)
6195       *d++ = *s++;
6196 
6197   return dst;
6198 }
6199 
6200 // memcpy exists to placate GCC.  Use memmove.
6201 void*
6202 memcpy(void *dst, const void *src, uint n)
6203 {
6204   return memmove(dst, src, n);
6205 }
6206 
6207 int
6208 strncmp(const char *p, const char *q, uint n)
6209 {
6210   while(n > 0 && *p && *p == *q)
6211     n--, p++, q++;
6212   if(n == 0)
6213     return 0;
6214   return (uchar)*p - (uchar)*q;
6215 }
6216 
6217 char*
6218 strncpy(char *s, const char *t, int n)
6219 {
6220   char *os;
6221 
6222   os = s;
6223   while(n-- > 0 && (*s++ = *t++) != 0)
6224     ;
6225   while(n-- > 0)
6226     *s++ = 0;
6227   return os;
6228 }
6229 
6230 // Like strncpy but guaranteed to NUL-terminate.
6231 char*
6232 safestrcpy(char *s, const char *t, int n)
6233 {
6234   char *os;
6235 
6236   os = s;
6237   if(n <= 0)
6238     return os;
6239   while(--n > 0 && (*s++ = *t++) != 0)
6240     ;
6241   *s = 0;
6242   return os;
6243 }
6244 
6245 
6246 
6247 
6248 
6249 
6250 int
6251 strlen(const char *s)
6252 {
6253   int n;
6254 
6255   for(n = 0; s[n]; n++)
6256     ;
6257   return n;
6258 }
6259 
6260 
6261 
6262 
6263 
6264 
6265 
6266 
6267 
6268 
6269 
6270 
6271 
6272 
6273 
6274 
6275 
6276 
6277 
6278 
6279 
6280 
6281 
6282 
6283 
6284 
6285 
6286 
6287 
6288 
6289 
6290 
6291 
6292 
6293 
6294 
6295 
6296 
6297 
6298 
6299 
