% master_slave_flip = 1;
% packet_burst = 10;
% COM1='COM3';
% COM2='COM4';
% 
% % get baudrate from settings.h
% [settings_file,errmsg] = fopen('../src/settings.h');
% strings = textscan(settings_file, '%s');
% idx1 = find(strcmp(strings{1}, 'UART_BAUD_RATE_SLAVE'), 1, 'first');
% idx2 = find(strcmp(strings{1}, 'UART_BAUD_RATE_MASTER'), 1, 'first');
% slave_baudrate = str2num(strings{1}{idx1+1});
% master_baudrate = str2num(strings{1}{idx2+1});
% fclose(settings_file);
% 
% if master_slave_flip == 1
%     master = serial(COM1,'BaudRate',master_baudrate);
%     slave = serial(COM2,'BaudRate',slave_baudrate);
% else
%     master = serial(COM2,'BaudRate',master_baudrate);
%     slave = serial(COM1,'BaudRate',slave_baudrate);
% end 
%  
% % j = batch('pararel_test','AttachedFiles',{'Master.m','Slave.m'})
% % 
% % wait(j);
% % load(j);
% 
% 
% 
% try
%     fopen(slave);
%     fopen(master);
%     msg=['master ',COM1];
%     disp(msg);
%     msg=['slave ',COM2];
%     disp(msg);
%     
%     for j = 1 : 5
%         for i = 1 :packet_burst
%             fprintf(slave,'%d$',i+1000);
%         end ;
%         pause(0.5);
%     end
%     
% %      for j = 1 : 5
%         for i = 1 :packet_burst
%             str = fscanf(master, '%d')
%         end ;
% %      end
%     fclose(slave);
%     fclose(master);
%     
% catch
%     msg = ['nepodarilo sa otvorit ', COM1,' alebo ', COM2 ' port.'];
%     disp(msg);
%     fclose(slave);
%     fclose(master);
% end


% range = [0, 2147483647];
% rng(500,'twister');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'simdTwister');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'combRecursive');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'multFibonacci');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'v5uniform');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'v5normal');
% % r = rng;
% r = randi(range,'uint32')
% rng(500,'v4');
% % r = rng;
% r = randi(range,'uint32')


% num = uint(uint32(next/65536)/32768)
% uint32(num)
% uint32(next/65536) mod 32768
% rem(int32(next/65536), 32768)
% 
% unsigned t1 = 0, t2 = 0;
% 
% 
%     unsigned b;
% 
%     b = t1 ^ (bitshift(t1,-1)) ^ (bitshift(t1,-6)) ^ (bitshift(t1,-7));
%     t1 = (bitshift(t1,-1)) | (bitshift(~b,-1));
% 
%     b = (t2 << 1) ^ (t2 << 2) ^ (t1 << 3) ^ (t2 << 4);
%     t2 = (t2 << 1) | (~b >> 31);
% 
%     t1 ^ t2
% rng(500,'twister')
% rand(1)
% next = next*1103515245 + 12345;
% 
% return (unsigned int) (next/65536) % 32768;
% load integersignal;
% next = 500;
% next = uint32(next)
% next = uint32(uint32(next)*uint32(1103515245)) 
% next = next + 12345
% next = next/32768;
% next = uint32(next);
% rem(next,32768);



% next = uint32(500);
% next = uint32(next*214013) + 2531011
% mod(uint32(next) , uint32(2^16))
% 
% next = uint64(500);
% next = typecast(((next*214013) + 2531011),'uint32')
% mod(uint32(next) , uint32(2^16))
% 
% next = uint64(500);
% next = uint32((next*22695477) + 1)
% mod(uint32(next) , uint32(2^16))
% 
% next = uint64(500);
% next = (next*1103515245) + 12345
% next = typecast(next,'uint32')
% rem(uint32(next(1)) , uint32(2^31))
% 
% arr = zeros(2^16,1,'uint8');
% next = uint64(500);
% for j=1:2
%     next = (next*1103515245);
%     next = typecast(next,'uint32');
%     next = uint64(next(1));
%     next = next+12345;
%     next = typecast(next,'uint32');
%     index = bitshift(next(1),-16);
%     arr(index) = arr(index)+1;
%     
%     next = uint64(next(1));
%     
% end
% plot(arr)





% t1 = uint16(1);
% t2 = uint16(6);
% 
% b = uint16(0);
% 
%     b = t1 ^ bitshift(t1, -2) ^ bitshift(t1, -6) ^ bitshift(t1, -7);
%     t1 = bitor( bitshift(t1, -1), bitshift(bitcmp(b,'uint16'), 31))
% 
%     b = bitshift(t2, 1) ^ bitshift(t2, 2) ^ bitshift(t1, 3) ^ bitshift(t2, 4);
%     t2 = bitor( bitshift(t2, 1), bitshift(bitcmp(b,'uint16'), -31))
% 
%     t1 ^ t2



next = uint64(500)
next = next*1103515245 + 12345;
asdf = typecast(next,'uint32');
next = uint64(asdf(1));
uint32(rem(uint32(next/65536),  32768))

asdf = typecast(next,'uint32');
next = uint64(asdf(1));
next = next*1103515245 + 12345;
asdf = typecast(next,'uint32');
next = uint64(asdf(1))
temp = (next/65536);
uint32(mod(uint32(temp), uint32( 32768)))

asdf = typecast(next,'uint32');
next = asdf(1);
next = next*1103515245 + 12345;
uint32(rem((next/65536),  32768))
asdf = typecast(next,'uint32');
next = asdf(1);
next = next*1103515245 + 12345;
uint32(rem((next/65536),  32768))