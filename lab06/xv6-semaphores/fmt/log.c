4150 #include "types.h"
4151 #include "defs.h"
4152 #include "param.h"
4153 #include "spinlock.h"
4154 #include "fs.h"
4155 #include "buf.h"
4156 
4157 // Simple logging. Each system call that might write the file system
4158 // should be surrounded with begin_trans() and commit_trans() calls.
4159 //
4160 // The log holds at most one transaction at a time. Commit forces
4161 // the log (with commit record) to disk, then installs the affected
4162 // blocks to disk, then erases the log. begin_trans() ensures that
4163 // only one system call can be in a transaction; others must wait.
4164 //
4165 // Allowing only one transaction at a time means that the file
4166 // system code doesn't have to worry about the possibility of
4167 // one transaction reading a block that another one has modified,
4168 // for example an i-node block.
4169 //
4170 // Read-only system calls don't need to use transactions, though
4171 // this means that they may observe uncommitted data. I-node and
4172 // buffer locks prevent read-only calls from seeing inconsistent data.
4173 //
4174 // The log is a physical re-do log containing disk blocks.
4175 // The on-disk log format:
4176 //   header block, containing sector #s for block A, B, C, ...
4177 //   block A
4178 //   block B
4179 //   block C
4180 //   ...
4181 // Log appends are synchronous.
4182 
4183 // Contents of the header block, used for both the on-disk header block
4184 // and to keep track in memory of logged sector #s before commit.
4185 struct logheader {
4186   int n;
4187   int sector[LOGSIZE];
4188 };
4189 
4190 struct log {
4191   struct spinlock lock;
4192   int start;
4193   int size;
4194   int busy; // a transaction is active
4195   int dev;
4196   struct logheader lh;
4197 };
4198 
4199 
4200 struct log log;
4201 
4202 static void recover_from_log(void);
4203 
4204 void
4205 initlog(void)
4206 {
4207   if (sizeof(struct logheader) >= BSIZE)
4208     panic("initlog: too big logheader");
4209 
4210   struct superblock sb;
4211   initlock(&log.lock, "log");
4212   readsb(ROOTDEV, &sb);
4213   log.start = sb.size - sb.nlog;
4214   log.size = sb.nlog;
4215   log.dev = ROOTDEV;
4216   recover_from_log();
4217 }
4218 
4219 // Copy committed blocks from log to their home location
4220 static void
4221 install_trans(void)
4222 {
4223   int tail;
4224 
4225   for (tail = 0; tail < log.lh.n; tail++) {
4226     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4227     struct buf *dbuf = bread(log.dev, log.lh.sector[tail]); // read dst
4228     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4229     bwrite(dbuf);  // write dst to disk
4230     brelse(lbuf);
4231     brelse(dbuf);
4232   }
4233 }
4234 
4235 // Read the log header from disk into the in-memory log header
4236 static void
4237 read_head(void)
4238 {
4239   struct buf *buf = bread(log.dev, log.start);
4240   struct logheader *lh = (struct logheader *) (buf->data);
4241   int i;
4242   log.lh.n = lh->n;
4243   for (i = 0; i < log.lh.n; i++) {
4244     log.lh.sector[i] = lh->sector[i];
4245   }
4246   brelse(buf);
4247 }
4248 
4249 
4250 // Write in-memory log header to disk.
4251 // This is the true point at which the
4252 // current transaction commits.
4253 static void
4254 write_head(void)
4255 {
4256   struct buf *buf = bread(log.dev, log.start);
4257   struct logheader *hb = (struct logheader *) (buf->data);
4258   int i;
4259   hb->n = log.lh.n;
4260   for (i = 0; i < log.lh.n; i++) {
4261     hb->sector[i] = log.lh.sector[i];
4262   }
4263   bwrite(buf);
4264   brelse(buf);
4265 }
4266 
4267 static void
4268 recover_from_log(void)
4269 {
4270   read_head();
4271   install_trans(); // if committed, copy from log to disk
4272   log.lh.n = 0;
4273   write_head(); // clear the log
4274 }
4275 
4276 void
4277 begin_trans(void)
4278 {
4279   acquire(&log.lock);
4280   while (log.busy) {
4281     sleep(&log, &log.lock);
4282   }
4283   log.busy = 1;
4284   release(&log.lock);
4285 }
4286 
4287 
4288 
4289 
4290 
4291 
4292 
4293 
4294 
4295 
4296 
4297 
4298 
4299 
4300 void
4301 commit_trans(void)
4302 {
4303   if (log.lh.n > 0) {
4304     write_head();    // Write header to disk -- the real commit
4305     install_trans(); // Now install writes to home locations
4306     log.lh.n = 0;
4307     write_head();    // Erase the transaction from the log
4308   }
4309 
4310   acquire(&log.lock);
4311   log.busy = 0;
4312   wakeup(&log);
4313   release(&log.lock);
4314 }
4315 
4316 // Caller has modified b->data and is done with the buffer.
4317 // Append the block to the log and record the block number,
4318 // but don't write the log header (which would commit the write).
4319 // log_write() replaces bwrite(); a typical use is:
4320 //   bp = bread(...)
4321 //   modify bp->data[]
4322 //   log_write(bp)
4323 //   brelse(bp)
4324 void
4325 log_write(struct buf *b)
4326 {
4327   int i;
4328 
4329   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
4330     panic("too big a transaction");
4331   if (!log.busy)
4332     panic("write outside of trans");
4333 
4334   for (i = 0; i < log.lh.n; i++) {
4335     if (log.lh.sector[i] == b->sector)   // log absorbtion?
4336       break;
4337   }
4338   log.lh.sector[i] = b->sector;
4339   struct buf *lbuf = bread(b->dev, log.start+i+1);
4340   memmove(lbuf->data, b->data, BSIZE);
4341   bwrite(lbuf);
4342   brelse(lbuf);
4343   if (i == log.lh.n)
4344     log.lh.n++;
4345   b->flags |= B_DIRTY; // XXX prevent eviction
4346 }
4347 
4348 
4349 
4350 // Blank page.
4351 
4352 
4353 
4354 
4355 
4356 
4357 
4358 
4359 
4360 
4361 
4362 
4363 
4364 
4365 
4366 
4367 
4368 
4369 
4370 
4371 
4372 
4373 
4374 
4375 
4376 
4377 
4378 
4379 
4380 
4381 
4382 
4383 
4384 
4385 
4386 
4387 
4388 
4389 
4390 
4391 
4392 
4393 
4394 
4395 
4396 
4397 
4398 
4399 
