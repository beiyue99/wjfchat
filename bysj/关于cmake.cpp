


DISTFILES +=  config.ini��

//�� qmake ��Ŀ�У�DISTFILES ����ָ���Ǳ����ļ����������ļ����ĵ��ȣ���
//ȷ����������Ŀ�������ߴ��ʱҲ�ᱻ������



add_custom_target(config_files SOURCES config.ini)

//����һ����Ϊ config_files ���Զ���Ŀ��
//SOURCES config.ini������ָ���� config.ini ��ΪĿ��ġ�Դ�ļ�����
//add_custom_target�����Խ� config.ini �ļ���ʽ��ӵ���Ŀ�У���������� CMake ���������Ƹ��ļ���ʹ�û�ַ���


//#CMAKE_BINARY_DIR����buildĿ¼  ��CMAKE_SOURCE_DIR����cmakelist���ڵ�Ŀ¼
//#CMAKE_RUNTIME_OUTPUT_DIRECTORY����ָ����ִ���ļ�������Ŀ¼