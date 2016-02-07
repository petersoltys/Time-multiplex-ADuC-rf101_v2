master_slave_flip = 1;
packet_burst = 10;

if master_slave_flip == 1
    slave = serial('COM3','BaudRate',128000);
    master = serial('COM4','BaudRate',128000);
else
    slave = serial('COM4','BaudRate',128000);
    master = serial('COM3','BaudRate',128000);
end 

% if master_slave_flip == 1
%     slave = serial('COM3','BaudRate',9600);
%     master = serial('COM5','BaudRate',128000);
% else
%     slave = serial('COM5','BaudRate',9600);
%     master = serial('COM3','BaudRate',128000);
% end 
 
% j = batch('pararel_test','AttachedFiles',{'Master.m','Slave.m'})
% 
% wait(j);
% load(j);



try
    fopen(slave)
    fopen(master)
    
     for i = 1 :2
         for i = 0 :packet_burst
            fprintf(slave,'message %d $',i);
         end
%          for i = 0 :packet_burst
            string = fscanf(master,'%s');
%          end
     end ;

    fclose(slave);
    fclose(master);

catch
    fclose(slave);
    fclose(master);
end
