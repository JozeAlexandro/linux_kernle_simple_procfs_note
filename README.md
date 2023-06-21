# linux_kernle_simple_procfs_note
Заметка к примеру procfs3.c 7 главы книги "The Linux Kernel Module Programming Guide"

Статья @todo 

Компиляция:
    make 
    
Очистка:
    make clean
    
Установка модуля:
    sudo insmod procfs_3_note.ko 
    
Удаление модуля:
    sudo rmmod procfs_3_note
    
Логи:
    sudo dmesg | grep procfs_3_note

Тест записи:
    echo "test string" > /proc/procfs_3_note 
    
Тест чтения:
    cat /proc/procfs_3_note
