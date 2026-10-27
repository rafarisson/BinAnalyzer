bina_add_library(session
    SOURCES
        include/bina/session/AnalysisSession.h
        AnalysisSession.cpp
    DEPENDS
        Bina::document
        Bina::parse
)
