NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

# Makefile sources block
SRCS_DIR = src
SRC_CGI_DIR = ${SRCS_DIR}/cgi
SRC_CONFIG_DIR = ${SRCS_DIR}/config
SRC_CORE_DIR = ${SRCS_DIR}/core
SRC_HTTP_DIR = ${SRCS_DIR}/http

# Makefile includes block
INCLUDES_DIR = include
INC_CGI_DIR = ${INCLUDES_DIR}/cgi
INC_CONFIG_DIR = ${INCLUDES_DIR}/config
INC_CORE_DIR = ${INCLUDES_DIR}/core
INC_HTTP_DIR = ${INCLUDES_DIR}/http

SRCS = ${SRC_CORE_DIR}/main.cpp \
	${SRC_CORE_DIR}/Socket.cpp \
	${SRC_CONFIG_DIR}/ConfigParser.cpp \
	${SRC_CONFIG_DIR}/InitHelpers.cpp \
	${SRC_CONFIG_DIR}/AuxiliarVerifing.cpp \
	${SRC_CONFIG_DIR}/Tokenizer.cpp \
	${SRC_CONFIG_DIR}/ParseConfigData.cpp \
	${SRC_CONFIG_DIR}/VerifingParseData.cpp \
	${SRC_CORE_DIR}/Multiplexer.cpp \
	${SRC_HTTP_DIR}/Request.cpp \
	${SRC_HTTP_DIR}/Autoindex.cpp \
	${SRC_HTTP_DIR}/HttpStatus.cpp \
	${SRC_HTTP_DIR}/Method.cpp \
	${SRC_HTTP_DIR}/Response.cpp \
	${SRCS_DIR}/utils/utils.cpp \
	${SRCS_DIR}/utils/refatorar_methods_post.cpp \
	${SRCS_DIR}/utils/refatorar_methods_get.cpp \
	${SRCS_DIR}/utils/helping_methods.cpp \
	${SRC_CGI_DIR}/CgiHandler.cpp \
	


INC = ${INC_CONFIG_DIR}/ConfigParser.hpp \
	${INC_CORE_DIR}/Socket.hpp \
	${INC_CORE_DIR}/Multiplexer.hpp \
	${INC_HTTP_DIR}/Request.hpp \
	${INC_HTTP_DIR}/HttpStatus.hpp \
	${INC_HTTP_DIR}/Method.hpp \
	${INC_HTTP_DIR}/Autoindex.hpp \
	${INC_HTTP_DIR}/MimeTypes.hpp \
	${INC_HTTP_DIR}/Response.hpp \
	${INCLUDES_DIR}/utils/utils.hpp \
	${INC_CGI_DIR}/CgiHandler.hpp \

OBJS = $(SRCS:.cpp=.o)

all : $(NAME)

$(NAME): $(OBJS)
	cd ${SRCS_DIR}
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp $(INC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	cd ${SRCS_DIR}
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
