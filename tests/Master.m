master_slave_flip = 0;

if master_slave_flip == 1
    master = serial('COM5','BaudRate',128000);
else
    master = serial('COM3','BaudRate',128000);
end 


try
    fopen(master)

    for i = 0 :100
        [A,count] = fscanf(master)
    end ;
    fclose(master);

catch
    fclose(master);
end
