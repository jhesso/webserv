##################################################
# COMPILER
##################################################

NAME = webserv

CC = c++

CFLAGS = -Wall -Wextra -Werror -g -std=c++17

##################################################
# FILES
##################################################

SRC = sources/main.cpp sources/Cluster.cpp sources/Configuration.cpp \
	sources/ConnectionManager.cpp sources/HttpRequest.cpp sources/HttpResponse.cpp \
	sources/Server.cpp sources/FileSystem.cpp sources/Folder.cpp sources/DirOrFile.cpp

OBJ = $(SRC:.cpp=.o)

##################################################
# MAKING
##################################################

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

##################################################
# CLEANING
##################################################

clean:
	rm -rf sources/*.o

fclean: clean
	rm -f $(NAME)

re: fclean all

tidy: all clean
