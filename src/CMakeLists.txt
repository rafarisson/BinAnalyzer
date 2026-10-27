# Camadas, de baixo para cima. Cada uma só depende das que vêm antes dela.
add_subdirectory(schema)        # tipos, campos e layout
add_subdirectory(format)        # valores decodificados e formatadores
add_subdirectory(parse)         # leitura do binário segundo o schema
add_subdirectory(document)      # documento editável (estrutura, tipos e formatos)
add_subdirectory(session)       # análise de um arquivo com um documento
add_subdirectory(io)            # importação e exportação (JSON, CSV)
add_subdirectory(model)         # item models (Qt Core) sobre documento e sessão
add_subdirectory(presentation)  # controladores de edição usados pela interface
add_subdirectory(app)           # serviços da aplicação e registro no QML
