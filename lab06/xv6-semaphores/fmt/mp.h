6300 // See MultiProcessor Specification Version 1.[14]
6301 
6302 struct mp {             // floating pointer
6303   uchar signature[4];           // "_MP_"
6304   void *physaddr;               // phys addr of MP config table
6305   uchar length;                 // 1
6306   uchar specrev;                // [14]
6307   uchar checksum;               // all bytes must add up to 0
6308   uchar type;                   // MP system config type
6309   uchar imcrp;
6310   uchar reserved[3];
6311 };
6312 
6313 struct mpconf {         // configuration table header
6314   uchar signature[4];           // "PCMP"
6315   ushort length;                // total table length
6316   uchar version;                // [14]
6317   uchar checksum;               // all bytes must add up to 0
6318   uchar product[20];            // product id
6319   uint *oemtable;               // OEM table pointer
6320   ushort oemlength;             // OEM table length
6321   ushort entry;                 // entry count
6322   uint *lapicaddr;              // address of local APIC
6323   ushort xlength;               // extended table length
6324   uchar xchecksum;              // extended table checksum
6325   uchar reserved;
6326 };
6327 
6328 struct mpproc {         // processor table entry
6329   uchar type;                   // entry type (0)
6330   uchar apicid;                 // local APIC id
6331   uchar version;                // local APIC verison
6332   uchar flags;                  // CPU flags
6333     #define MPBOOT 0x02           // This proc is the bootstrap processor.
6334   uchar signature[4];           // CPU signature
6335   uint feature;                 // feature flags from CPUID instruction
6336   uchar reserved[8];
6337 };
6338 
6339 struct mpioapic {       // I/O APIC table entry
6340   uchar type;                   // entry type (2)
6341   uchar apicno;                 // I/O APIC id
6342   uchar version;                // I/O APIC version
6343   uchar flags;                  // I/O APIC flags
6344   uint *addr;                  // I/O APIC address
6345 };
6346 
6347 
6348 
6349 
6350 // Table entry types
6351 #define MPPROC    0x00  // One per processor
6352 #define MPBUS     0x01  // One per bus
6353 #define MPIOAPIC  0x02  // One per I/O APIC
6354 #define MPIOINTR  0x03  // One per bus interrupt source
6355 #define MPLINTR   0x04  // One per system interrupt source
6356 
6357 
6358 
6359 
6360 
6361 
6362 
6363 
6364 
6365 
6366 
6367 
6368 
6369 
6370 
6371 
6372 
6373 
6374 
6375 
6376 
6377 
6378 
6379 
6380 
6381 
6382 
6383 
6384 
6385 
6386 
6387 
6388 
6389 
6390 
6391 
6392 
6393 
6394 
6395 
6396 
6397 
6398 
6399 
