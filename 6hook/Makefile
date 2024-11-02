# VERSION 3
CXX = g++
TARGET = ab_fiber
# OBJ = ab_fiber.o fd_manager.o fiber.o hook.o ioscheduler.o scheduler.o thread.o timer.o
 
# 定义源文件
SOURCES := ab_fiber.cpp fd_manager.cpp fiber.cpp hook.cpp ioscheduler.cpp scheduler.cpp thread.cpp timer.cpp

# 从源文件生成目标文件列表
OBJECTS := $(SOURCES:.cpp=.o)
OBJDIR := obj
OBJS := $(addprefix $(OBJDIR)/, $(OBJECTS))

# 编译选项，显示所有的warning
CXXLAGS = -c -std=c++17
LDFLAGS = -pthread -ldl
# $@表示的就是冒号前面的TARGET，$^表示的是冒号后OBJ的全部.o依赖文件
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)	# 链接的顺序非常重要！！！
 
# $<表示指向%.cpp依赖的第一个，但是这里依赖只有一个
# $@表示指向%.o
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXLAGS) $< -o $@
 
# 为了防止文件夹中存在一个文件叫clean
.PHONY: clean
 
# -f表示强制删除，此处表示删除所有的.o文件和TARGET文件
clean:
	rm -rf $(OBJDIR) $(TARGET)