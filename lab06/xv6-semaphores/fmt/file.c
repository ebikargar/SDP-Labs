5200 //
5201 // File descriptors
5202 //
5203 
5204 #include "types.h"
5205 #include "defs.h"
5206 #include "param.h"
5207 #include "fs.h"
5208 #include "file.h"
5209 #include "spinlock.h"
5210 
5211 struct devsw devsw[NDEV];
5212 struct {
5213   struct spinlock lock;
5214   struct file file[NFILE];
5215 } ftable;
5216 
5217 void
5218 fileinit(void)
5219 {
5220   initlock(&ftable.lock, "ftable");
5221 }
5222 
5223 // Allocate a file structure.
5224 struct file*
5225 filealloc(void)
5226 {
5227   struct file *f;
5228 
5229   acquire(&ftable.lock);
5230   for(f = ftable.file; f < ftable.file + NFILE; f++){
5231     if(f->ref == 0){
5232       f->ref = 1;
5233       release(&ftable.lock);
5234       return f;
5235     }
5236   }
5237   release(&ftable.lock);
5238   return 0;
5239 }
5240 
5241 
5242 
5243 
5244 
5245 
5246 
5247 
5248 
5249 
5250 // Increment ref count for file f.
5251 struct file*
5252 filedup(struct file *f)
5253 {
5254   acquire(&ftable.lock);
5255   if(f->ref < 1)
5256     panic("filedup");
5257   f->ref++;
5258   release(&ftable.lock);
5259   return f;
5260 }
5261 
5262 // Close file f.  (Decrement ref count, close when reaches 0.)
5263 void
5264 fileclose(struct file *f)
5265 {
5266   struct file ff;
5267 
5268   acquire(&ftable.lock);
5269   if(f->ref < 1)
5270     panic("fileclose");
5271   if(--f->ref > 0){
5272     release(&ftable.lock);
5273     return;
5274   }
5275   ff = *f;
5276   f->ref = 0;
5277   f->type = FD_NONE;
5278   release(&ftable.lock);
5279 
5280   if(ff.type == FD_PIPE)
5281     pipeclose(ff.pipe, ff.writable);
5282   else if(ff.type == FD_INODE){
5283     begin_trans();
5284     iput(ff.ip);
5285     commit_trans();
5286   }
5287 }
5288 
5289 
5290 
5291 
5292 
5293 
5294 
5295 
5296 
5297 
5298 
5299 
5300 // Get metadata about file f.
5301 int
5302 filestat(struct file *f, struct stat *st)
5303 {
5304   if(f->type == FD_INODE){
5305     ilock(f->ip);
5306     stati(f->ip, st);
5307     iunlock(f->ip);
5308     return 0;
5309   }
5310   return -1;
5311 }
5312 
5313 // Read from file f.
5314 int
5315 fileread(struct file *f, char *addr, int n)
5316 {
5317   int r;
5318 
5319   if(f->readable == 0)
5320     return -1;
5321   if(f->type == FD_PIPE)
5322     return piperead(f->pipe, addr, n);
5323   if(f->type == FD_INODE){
5324     ilock(f->ip);
5325     if((r = readi(f->ip, addr, f->off, n)) > 0)
5326       f->off += r;
5327     iunlock(f->ip);
5328     return r;
5329   }
5330   panic("fileread");
5331 }
5332 
5333 
5334 
5335 
5336 
5337 
5338 
5339 
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Write to file f.
5351 int
5352 filewrite(struct file *f, char *addr, int n)
5353 {
5354   int r;
5355 
5356   if(f->writable == 0)
5357     return -1;
5358   if(f->type == FD_PIPE)
5359     return pipewrite(f->pipe, addr, n);
5360   if(f->type == FD_INODE){
5361     // write a few blocks at a time to avoid exceeding
5362     // the maximum log transaction size, including
5363     // i-node, indirect block, allocation blocks,
5364     // and 2 blocks of slop for non-aligned writes.
5365     // this really belongs lower down, since writei()
5366     // might be writing a device like the console.
5367     int max = ((LOGSIZE-1-1-2) / 2) * 512;
5368     int i = 0;
5369     while(i < n){
5370       int n1 = n - i;
5371       if(n1 > max)
5372         n1 = max;
5373 
5374       begin_trans();
5375       ilock(f->ip);
5376       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
5377         f->off += r;
5378       iunlock(f->ip);
5379       commit_trans();
5380 
5381       if(r < 0)
5382         break;
5383       if(r != n1)
5384         panic("short filewrite");
5385       i += r;
5386     }
5387     return i == n ? n : -1;
5388   }
5389   panic("filewrite");
5390 }
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
