mkdir no_name
copy build\Release\no_name.exe no_name\
copy build\Release\no_name_editor.exe no_name\
copy build\Release\SDL2.dll no_name\
copy build\Release\SDL2_mixer.dll no_name\
xcopy data\levels no_name\data\levels\ /E /I /Y
xcopy data\music no_name\data\music\ /E /I /Y
xcopy data\sounds no_name\data\sounds\ /E /I /Y
copy  data\spritesheet.png no_name\data\
copy  data\afi.jpg no_name\data\
copy  data\LiberationMono-Regular.ttf no_name\data\
copy  data\SuperMarioWorldTextBoxRegular-Y86j.ttf no_name\data\