//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. 107186  Name: Vitor Santos
//   N.Mec. 108969  Name: Rodrigo Aguiar
//
//aa
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadh_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elapsed_time.h"


//
// static configuration
//

#define _max_word_size_ 32
#define _init_hash_size_ 1000
#define _DEBUG_HASH_ 0
#define _SHOW_NODES 0
#define _VER_REPRESENTANTES_ 0


//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;    // link to the next adjacency list node
  hash_table_node_t *vertex; // the other vertex ( o proximo vertice adjacente )
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_]; // the word
  hash_table_node_t *next;    // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;      // head of the linked list of adjancency edges
  int visited;                 // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous; // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;   // the size of the hash table array
  unsigned int number_of_entries; // the number of entries in the hash table
  unsigned int number_of_edges;   // number of edges (for information purposes only)
  unsigned int num_heads;         // número de cabeças de lista ligada não-nulas na tabela de dispersão a utilizar no cálculo de nºheads/nºcollisions
                                  // nºcollisions = nºentries - nºheads
  unsigned int sum_degrees;       // soma dos graus de todos os nós do grafo ; utilizar para calcular grau médio
  unsigned int n_connected_components; // número de componentes conexos
  hash_table_node_t **heads;      // the heads of the linked lists
};

//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)calloc(1, sizeof(adjacency_node_t));
  if (node == NULL)
  {
    fprintf(stderr, "allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)calloc(1,sizeof(hash_table_node_t));
  node->representative = node;
  node->number_of_edges = 0;
  node->number_of_vertices = 1;
  node->visited = -1;
  // calloc inicializa ponteiros com valor NULO
  if (node == NULL)
  {
    fprintf(stderr, "allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}

//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if (table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i, j;

    for (i = 0u; i < 256u; i++)
      for (table[i] = i, j = 0u; j < 8u; j++)
        if (table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while (*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc; // é necessario dividir pelo hash_table_size ao longo que se vai dando resize
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;

  hash_table = (hash_table_t *)calloc(1,sizeof(hash_table_t));
  if (hash_table == NULL)
  {
    fprintf(stderr, "create_hash_table: out of memory\n");
    exit(1);
  }
  hash_table_node_t **heads = (hash_table_node_t **)calloc(_init_hash_size_, sizeof(hash_table_node_t*));
  if (heads == NULL)
  {
    fprintf(stderr, "create_hash_table: out of memory for heads array\n");
    exit(1);
  }
  // Alocar array de heads com tamanho de tabela inicial _init_hash_size_

  hash_table->heads = heads;
  hash_table->hash_table_size = _init_hash_size_;
  hash_table->num_heads = 0;
  hash_table->number_of_edges = 0;
  hash_table->number_of_entries = 0;

  return hash_table;
}
int rewrite_file = 0;
static void hash_table_info(hash_table_t *hash_table)
{
  FILE *fp = NULL;
  if (rewrite_file == 0){
   fp = fopen("hash_table_info.txt", "w+");
   rewrite_file++;
  }
  else{
    //printf("Appending to file...");
    fp = fopen("hash_table_info.txt", "aw");
  }
    fprintf(fp,"\n +--------------------+ \n");
    fprintf(fp,"HASH TABLE SIZE: %d\n",hash_table->hash_table_size);
    fprintf(fp,"Nº HASH TABLE ENTRIES: %d\n",hash_table->number_of_entries);
    unsigned int num_of_heads = hash_table->num_heads;
    unsigned int empty_nodes = 0;
    fprintf(fp,"Nº HASH TABLE HEADS: %d\n",num_of_heads);
    fprintf(fp,"Number of Collisions: %d \n",hash_table->number_of_entries - num_of_heads);
    fprintf(fp,"Collision ratio (%%): %f \n", 100*(hash_table->number_of_entries - num_of_heads)/(double)hash_table->number_of_entries);
    for (int i = 0; i < hash_table->hash_table_size;i++)
    {
      if (hash_table->heads[i] == NULL)
      {
        empty_nodes++;
      }
    }
    fprintf(fp,"Nº EMPTY HEADS NODES: %d\n",empty_nodes);
    fprintf(fp,"Empty heads nodes ratio: %.2f %%",100*empty_nodes/(double)hash_table->hash_table_size);
    int size_of_linked_lists[hash_table->hash_table_size];
    // initialize all size of linked list indexes to 0
    #if _SHOW_NODES
    for (int i = 0; i < hash_table->hash_table_size; i++)
    {
      size_of_linked_lists[i] = 0;
    }
    // calculate number of nodes per linked list
    for (int i = 0; i < hash_table->hash_table_size; i++)
    {
      node = hash_table->heads[i];
      while (node != NULL)
      {
        node = node->next;
        size_of_linked_lists[i]++;
      }
    }
    // print amount of nodes per linked list
    fprintf(fp,"Size of Linked Lists: \n");
    for (int i=0; i<hash_table->hash_table_size; i++)
    {
      fprintf(fp,"%d ",size_of_linked_lists[i]);
    }
    #endif
fclose(fp);
// 
}
static void hash_table_grow(hash_table_t *hash_table)
{
  hash_table_info(hash_table);
  hash_table_node_t *node;                // node atual para tranversal da linked list
  hash_table_node_t *last_node;
  hash_table_node_t *nn; // next node
  unsigned int prev_hash_table_size = hash_table->hash_table_size;
  hash_table->hash_table_size = hash_table->hash_table_size * 2;                                                           // increase size by 2

  hash_table_node_t **new_heads = (hash_table_node_t **)calloc(hash_table->hash_table_size, sizeof(hash_table_node_t *)); // allocate space for the new heads

  if (new_heads == NULL)
  {
    fprintf(stderr, "grow_hash_table: out of memory for new heads array\n");
    exit(1);
  }

  unsigned int i, new_index;
  hash_table->num_heads = 0;
  for (i = 0; i < prev_hash_table_size; i++)
  {
    node = hash_table->heads[i]; // node assume o head inicialmente
    while (node != NULL)
    {
      nn = node->next;
      node->next = NULL;
      new_index = crc32(node->word) % hash_table->hash_table_size; // calcula o indice para onde vai a palavra que se encontra no node
      if (new_heads[new_index] == NULL)
      {                                                 // se o heads do array novo ainda não tiver nenhum node neste indice, este node pode passar a ser o head
        new_heads[new_index] = node; // copia endereço do nó a ser reposicionado para nova posição na tabela de dispersão redimensionada
        hash_table->num_heads++;
      }
      else
      { // caso já exista um node neste indice do new_heads , temos de percorrer a linked list até ao fim para inserir o node
        last_node = new_heads[new_index];
        while(last_node->next != NULL)
        {
          last_node = last_node->next;
        }
        last_node->next = node;
      }
      node = nn; // avança para o próximo nó da lista ligada 
    }
  }
  free(hash_table->heads);
  hash_table->heads = new_heads; // o array de heads passa a ser o array de heads novo
}

  static void hash_table_free(hash_table_t * hash_table)
  {
    unsigned int i;
    // iterate all elements of hash_table->heads
    for (i = 0; i < hash_table->hash_table_size; i++)
    {
      if (hash_table->heads[i] != NULL)
      {
        hash_table_node_t *node = hash_table->heads[i];
        hash_table_node_t *next_node;
        while (node != NULL)
        {
          adjacency_node_t *n = node->head;
          adjacency_node_t *nn = NULL;
          while (n != NULL)
          {
            nn = n->next;
            free_adjacency_node(n);
            n = nn;
          }
          next_node = node->next;
          free_hash_table_node(node);
          node = next_node;
        }
      }
    }
    free(hash_table->heads);
    free(hash_table);   // free the hash table pointer itself
  }

  static hash_table_node_t *find_word(hash_table_t * hash_table, const char *word, int insert_if_not_found)
  {
    double ratio = ( (double)hash_table->number_of_entries)/((double)(hash_table->hash_table_size));
    if(insert_if_not_found == 1 && ratio >= 0.50)
      hash_table_grow(hash_table);
    hash_table_node_t *node, *new_node;
    unsigned int i;          // index possível em que a palavra está

    if (strlen(word) > _max_word_size_)
    {
      fprintf(stderr, "find_word: word too long: %s\n", word);
      exit(1);
    }


    i = crc32(word) % hash_table->hash_table_size; // hash code
    node = hash_table->heads[i];
    //
    if (node == NULL)
    {
      if (insert_if_not_found == 1)
      {
        node = allocate_hash_table_node();
        strcpy(node->word, word);
        node->next = NULL;
        hash_table->heads[i] = node;
        hash_table->n_connected_components = ++hash_table->number_of_entries;
        hash_table->num_heads++;
      }
    }
    else // se o nó inicial existir
    {
      while (node->next != NULL)
      {
        if (strcmp(node->word, word) == 0) // vai comparando os nós da linked list , se a palavra for igual retorna o nó onde ela está
        {
          return node;
        }
        node = node->next;
        // enquanto não chegar ao fim da linked list, continua a percorrer
      }
      if(strcmp(node->word, word) == 0)
        return node;
      // se a palavra não existe na tabela de dispersão, as linhas seguintes são executadas
      if (insert_if_not_found == 1) 
      {
        new_node = allocate_hash_table_node();
        strcpy(new_node->word, word);
        new_node->next = NULL;
        node->next = new_node;
        node = new_node;
        hash_table->n_connected_components = ++hash_table->number_of_entries;
      }
      else
      {
        node = NULL;
      }
    }
    return node;
  }

  //
  // add edges to the word ladder graph (mostly do be done)
  //

  static hash_table_node_t *find_representative(hash_table_node_t * node)
  {
    hash_table_node_t *repr, *next_node, *curr_node;

    // Encontrar nó representante
    for(repr = node;repr != repr->representative;repr = repr->representative)
      ;
    // Compressão de caminho, corrigindo os representantes. Encurta tempo necessário para encontrar representante
    for(curr_node = node;curr_node != repr;curr_node = next_node)
    {
      next_node = curr_node->representative;
      curr_node->representative = repr;
    }
    return repr;
  }

  static void add_edge(hash_table_t * hash_table, hash_table_node_t * from, const char *word)
  {
    hash_table_node_t *to, *from_representative, *to_representative;
    adjacency_node_t *link;

    to = find_word(hash_table, word, 0);
    if(from == NULL || to == NULL)
      return;
    
    // Adicionar nó "from" à lista de adjacência do nó "to"

    adjacency_node_t *n;


    if(to->head == NULL)
    {
      link = allocate_adjacency_node();
      link->vertex = from;
      link->next = NULL; // Caso calloc não inicialize link->next para NULL por algum motivo
      to->head = link;
    }
    else
    {
      n = to->head;
      while(n->next != NULL)
      {
        if(n->vertex == from)
          return; // aresta já foi estabelecida
        n = n->next;
      }
      if(n->vertex == from)
        return;
      link = allocate_adjacency_node();
      link->vertex = from;
      link->next = NULL; // Caso calloc não inicialize link->next para NULL por algum motivo
      n->next = link;
    }

    // Adicionar nó "to" à lista de adjacência do nó "from"

    if(from->head == NULL)
    {
      link = allocate_adjacency_node();
      link->vertex = to;
      link->next = NULL; // Caso calloc não inicialize link->next para NULL por algum motivo
      from->head = link;
    }
    else
    {
      n = from->head;
      while(n->next != NULL)
      {
        if(n->vertex == to)
          return;
        n = n->next;
      } //
      if(n->vertex == to)
        return;
      link = allocate_adjacency_node();
      link->vertex = to;
      link->next = NULL; // Caso calloc não inicialize link->next para NULL por algum motivo
      n->next = link;
    }

    hash_table->number_of_edges++;
    hash_table->sum_degrees += 2;

    from_representative = find_representative(from);
    to_representative = find_representative(to);
    from_representative->number_of_edges++;

    if(to_representative != from_representative)
    {
      from_representative->number_of_edges += to_representative->number_of_edges;
      from_representative->number_of_vertices += to_representative->number_of_vertices;
      to_representative->representative = from_representative;
      hash_table->n_connected_components--;
    }
  }

  //
  // genx
  //
  // man utf8 for details on the uft8 encoding
  //

  static void break_utf8_string(const char *word, int *individual_characters)
  {
    int byte0, byte1;

    while (*word != '\0')
    {
      byte0 = (int)(*(word++)) & 0xFF;
      if (byte0 < 0x80)
        *(individual_characters++) = byte0; // plain ASCII character
      else
      {
        byte1 = (int)(*(word++)) & 0xFF;
        if ((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
        {
          fprintf(stderr, "break_utf8_string: unexpected UFT-8 character\n");
          exit(1);
        }
        *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
      }
    }
    *individual_characters = 0; // mark the end!
  }

  static void make_utf8_string(const int *individual_characters, char word[_max_word_size_])
  {
    int code;

    while (*individual_characters != 0)
    {
      code = *(individual_characters++);
      if (code < 0x80)
        *(word++) = (char)code;
      else if (code < (1 << 11))
      { // unicode -> utf8
        *(word++) = 0b11000000 | (code >> 6);
        *(word++) = 0b10000000 | (code & 0b00111111);
      }
      else
      {
        fprintf(stderr, "make_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
    }
    *word = '\0'; // mark the end
  }

  static void similar_words(hash_table_t * hash_table, hash_table_node_t * from)
  {
    static const int valid_characters[] =
        {                                                                                          // unicode!
         0x2D,                                                                                     // -
         0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,             // A B C D E F G H I J K L M
         0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,             // N O P Q R S T U V W X Y Z
         0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,             // a b c d e f g h i j k l m
         0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,             // n o p q r s t u v w x y z
         0xC1, 0xC2, 0xC9, 0xCD, 0xD3, 0xDA,                                                       // Á Â É Í Ó Ú
         0xE0, 0xE1, 0xE2, 0xE3, 0xE7, 0xE8, 0xE9, 0xEA, 0xED, 0xEE, 0xF3, 0xF4, 0xF5, 0xFA, 0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
         0};
    int i, j, k, individual_characters[_max_word_size_];
    char new_word[2 * _max_word_size_];

    break_utf8_string(from->word, individual_characters);
    for (i = 0; individual_characters[i] != 0; i++)
    {
      k = individual_characters[i];
      for (j = 0; valid_characters[j] != 0; j++)
      {
        individual_characters[i] = valid_characters[j];
        make_utf8_string(individual_characters, new_word);
        // avoid duplicate cases
        if (strcmp(new_word, from->word) > 0)
          add_edge(hash_table, from, new_word);
      }
      individual_characters[i] = k;
    }
  }


  // Repõe os valores visited e previous dos nós percorridos por uma breadth search

  static void clean_traversal(hash_table_node_t **list_of_vertices, int maximum_num)
  {
    int i;
    for(i = 0; i < maximum_num; i++)
      if(list_of_vertices[i] != NULL)
      {
        list_of_vertices[i]->previous = NULL;
        list_of_vertices[i]->visited = -1;
      }
  }

  //
  // breadth-first search (to be done)
  //
  // returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
  //

  static int breadh_first_search(int maximum_number_of_vertices, hash_table_node_t **list_of_vertices, hash_table_node_t *origin, hash_table_node_t *goal)
  {
    // Implementação utilizando um Queue de tamanho maximum_number_of_vertices e endereço base 
    // guardado no ponteiro list_of_vertices

    // O array list_of_vertices a utilizar para o Queue deverá ser alocado previamente à chamada da função.

    // Devolve distância entre palavra origem e palavra destino

    int en_idx = 1; // enqueue index
    int de_idx = 0; // dequeue index

    hash_table_node_t* curr_node, *n_hash;
    adjacency_node_t* n;

    list_of_vertices[de_idx] = origin;
    origin->visited = 0;

    // Caso excecional

    if(origin == goal)
      return origin->visited; 

    while(en_idx < maximum_number_of_vertices && en_idx != de_idx)
    {
      curr_node = list_of_vertices[de_idx++]; // dequeue

      // enqueue nós adjacentes a curr_node não-marcados
      for(n = curr_node->head; n != NULL && en_idx < maximum_number_of_vertices; n = n->next)
        if(n->vertex->visited < 0)
        {
          n_hash = n->vertex;
          n_hash->previous = curr_node; // guardar nó anterior do caminho mais curto
          list_of_vertices[en_idx++] = n_hash; // enqueue
          n_hash->visited = curr_node->visited + 1; // marcar nó
          if(n_hash == goal)
            return n_hash->visited;
        }
    }
    return -1; // No caso de não existir caminho entre origin e goal
  }

  //
  // list all vertices belonging to a connected component (complete this)
  //

  static void list_connected_component(hash_table_t * hash_table, const char *word)
  {
    int i, maximum_vertice_num;
    hash_table_node_t* origin = find_word(hash_table, word, 0);
    if(origin == NULL)
    {
      printf(">>> Palavra não existe na tabela de dispersão\n");
      return;
    }
    maximum_vertice_num = find_representative(origin)->number_of_vertices;
    hash_table_node_t **list_of_vertices = (hash_table_node_t **)calloc(maximum_vertice_num, sizeof(hash_table_node_t *));

    breadh_first_search(maximum_vertice_num, list_of_vertices, origin, NULL); // Preencher list_of_vertices com nós percorridos

    // Listar componente conexo
    for(i = 0; i < maximum_vertice_num; i++)
      printf("%d: [%s]\n", i, list_of_vertices[i]->word);
    
    clean_traversal(list_of_vertices, maximum_vertice_num); // Repôr parâmetros "visited" dos nós percorridos


    free(list_of_vertices);
    printf(">>> Tamanho do componente conexo de %s (R: %s): %d \n",origin->word,find_representative(origin)->word, find_representative(origin)->number_of_vertices);
  }

  //
  // compute the diameter of a connected component (optional)
  //

  static int largest_diameter;
  static hash_table_node_t *largest_diameter_example;

  static int connected_component_diameter(hash_table_node_t * node, int print_path)
  {
    int max_num_vert, j, diameter = 0;

    if(node == NULL)
      return -1;

    // Primeira BFS
    max_num_vert = find_representative(node)->number_of_vertices;

    hash_table_node_t **connected_component_vertices = calloc(max_num_vert, sizeof(hash_table_node_t *));
    breadh_first_search(max_num_vert, connected_component_vertices, node, NULL);
    clean_traversal(connected_component_vertices, max_num_vert);

    // BFS para cada nó do componente conexo

    hash_table_node_t **list_of_vertices = calloc(max_num_vert, sizeof(hash_table_node_t *));
    hash_table_node_t **longest_path = calloc(max_num_vert, sizeof(hash_table_node_t *));
    longest_path[0] = node;

    for(int i = 0; i < max_num_vert; i++)
    {
      breadh_first_search(max_num_vert, list_of_vertices, connected_component_vertices[i], NULL);
      if(list_of_vertices[max_num_vert - 1]->visited > diameter)
      {
        diameter = list_of_vertices[max_num_vert - 1]->visited;

        // Registar caminho mais comprido
        hash_table_node_t *n = list_of_vertices[max_num_vert - 1];
        for(j = 0; j != diameter + 1; j++)
        {
          longest_path[j] = n;
          n = n->previous;
        }
      }
      clean_traversal(list_of_vertices, max_num_vert);
    }
    if(print_path)
      for(j = 0; j != diameter + 1; j++)
      {
        printf("%d: [%s]\n", j, longest_path[j]->word);
      }
    free(list_of_vertices);
    free(connected_component_vertices);
    free(longest_path);

    return diameter;
  }

  //
  // find the shortest path from a given word to another given word
  //

  static void path_finder(hash_table_t * hash_table, const char *from_word, const char *to_word)
  {
    int len, i, maximum_vertice_num;

    hash_table_node_t* goal = find_word(hash_table, from_word, 0); // Inversão semântica
    hash_table_node_t* origin = find_word(hash_table, to_word, 0);

    if(origin == NULL)
    {
      printf(">>> %s não existe na tabela de dispersão\n", to_word);
      return;
    }

    if(goal == NULL)
    {
      printf(">>> %s não existe na tabela de dispersão\n", from_word);
      return; 
    }
    printf(">>> Comparing %s and %s\n", find_representative(goal)->word, find_representative(origin)->word);
    if(goal->representative != origin->representative)
    {
      printf(">>> %s e %s não compõem o mesmo componente conexo\n", from_word, to_word);
      return;
    }

    maximum_vertice_num = origin->representative->number_of_vertices;
    hash_table_node_t **list_of_vertices = (hash_table_node_t **)calloc(maximum_vertice_num, sizeof(hash_table_node_t *));

    len = breadh_first_search(maximum_vertice_num, list_of_vertices, origin, goal);

    
    if(len < 0) // s == -1 => breadh_first_search não encontrou nó goal
    {
      printf(">> Word ladder impossível de %s para %s\n", from_word, to_word);
      clean_traversal(list_of_vertices, maximum_vertice_num); // Repôr parâmetros "visited" dos nós percorridos
      free(list_of_vertices);
      return;
    }
    

    // Percorrer caminho mais curto entre a palavra from_word e to_word

    hash_table_node_t* n;
    i = 0;
    for(n = goal; n != origin; n = n->previous)
    {
      printf("%d: [%s] \n", i, n->word);
      i++;
    }
    printf("%d: [%s]\n", i, n->word);
    printf(">>> Comprimento de word ladder: %d \n", len);
    
    clean_traversal(list_of_vertices, maximum_vertice_num); // Repôr parâmetros "visited" dos nós percorridos
    free(list_of_vertices);
  }

  //
  // some graph information (optional)
  //

  static void graph_info(hash_table_t * hash_table)
  {
    double col_ratio = (double)(hash_table->number_of_entries - hash_table->num_heads)/(double)(hash_table->number_of_entries);
    double avg_deg = (double)(hash_table->sum_degrees)/(double)(hash_table->number_of_entries);

    FILE *fp = fopen("output_diameters.csv", "w"); // A utilizar nos histogramas de diâmetros
    FILE *fp_2 = fopen("output_edges_vertices.csv", "w"); // A utilizar no histograma de densidades

    printf("\n>>> GRAPH INFO <<<\n\n");
    printf(">>> Number of entries: %i\n", hash_table->number_of_entries);
    printf(">>> Number of heads: %i\n", hash_table->num_heads);
    printf(">>> Number of collisions: %i\n", hash_table->number_of_entries - hash_table->num_heads);
    printf(">>> Collision ratio: %f %%\n", col_ratio*100);
    printf(">>> Number of edges: %i\n" , hash_table->number_of_edges);
    printf(">>> Average degree: %f\n", avg_deg); 
    printf(">>> Number of connected components: %i \n", hash_table->n_connected_components);

    largest_diameter = 0;
    hash_table_node_t *largest_connected_component = NULL;
    int largest_co_comp_size = 0;
    double avg_diameter = 0;
    double count = 0; // To count number of connected components with more than 1 node

    for(unsigned int i = 0; i < hash_table->hash_table_size; i++)
    {
      hash_table_node_t *n;
      for(n = hash_table->heads[i]; n != NULL; n = n->next)
      {
        if(n->representative == n)
        {
          // Registar número de nós e arestas no componente conexo
          fprintf(fp_2, "%d %d\n", n->number_of_edges, n->number_of_vertices);
          // Contagem de componentes conexos com mais de um nó
          if(n->head != NULL)
            count++;
          // Determinar maior componente conexo
          if(n->number_of_vertices > largest_co_comp_size)
          {
            largest_co_comp_size = n->number_of_vertices;
            largest_connected_component = n;
          }
          // Determinar maior diâmetro
          int d = connected_component_diameter(n, 0);
          fprintf(fp, "%d\n", d); // Registar
          avg_diameter += d;
          if(d > largest_diameter)
          {
            largest_diameter = d;
            largest_diameter_example = n;
          }
        }
      }
    }
    fclose(fp);
    fclose(fp_2);

    double avg_diameter_total = avg_diameter/(double)(hash_table->n_connected_components);
    double avg_diameter_no_lone = avg_diameter/count;
    printf(">>> Largest connected component: %s ; size: %d\n", largest_connected_component->word, largest_co_comp_size);
    printf(">>> Largest diameter (R: %s): %d\n", largest_diameter_example->word, largest_diameter);
    printf(">>> Average connected component diameter: %f\n", avg_diameter_total);
    printf(">>> Avg. connected component diameter (no isolated nodes): %f\n", avg_diameter_no_lone);
    printf(">>> Number of isolated nodes: %d\n", hash_table->n_connected_components - (unsigned int)count);

    printf("\n");
  }

  // Output adjacency lists for Gephi

  static void output_adjacency_lists(hash_table_t *hash_table)
  {
    unsigned int i;
    hash_table_node_t *n;
    adjacency_node_t *a;

    FILE *fp = fopen("output_adjacency_lists.csv", "w");

    fprintf(fp, "source, target\n");

    // Percorrer todos os nós e listas de adjacência respetivas

    for(i = 0; i < hash_table->hash_table_size; i++)
      for(n = hash_table->heads[i]; n != NULL; n = n->next)
        for(a = n->head; a != NULL; a = a->next)
          fprintf(fp, "%s, %s\n", n->word, a->vertex->word);

    fclose(fp);
  }


  //
  // main program
  //

  int main(int argc, char **argv)
  {
    char word[100], from[100], to[100];
    hash_table_t *hash_table;
    hash_table_node_t *node;
    unsigned int i;
    int command;
    FILE *fp;

    // initialize hash table
    hash_table = hash_table_create();
    // read words
    fp = fopen((argc < 2) ? "wordlist-four-letters.txt" : argv[1], "rb");
    if (fp == NULL)
    {
      fprintf(stderr, "main: unable to open the words file\n");
      exit(1);
    }
    double t1 = cpu_time();
    while (fscanf(fp, "%99s", word) == 1)
    {
      (void)find_word(hash_table, word, 1);
#if _DEBUG_HASH_
      //printf(">> %s\n", find_word(hash_table,word,0));
#endif
    }
    hash_table_info(hash_table);
#if _DEBUG_HASH_
    //hash_table_grow(hash_table);
    double t2 = cpu_time();

    printf("HASH TABLE SIZE: %d\n",hash_table->hash_table_size);
    printf("Nº HASH TABLE ENTRIES: %d\n",hash_table->number_of_entries);
    printf("Time to build hash table: %e\n",t2-t1);

    unsigned int num_of_heads = hash_table->num_heads;
    printf("Nº HASH TABLE HEADS: %d\n",num_of_heads);
    printf("Number of Collisions: %d\n",hash_table->number_of_entries - num_of_heads);
    printf("Collision ratio (%%): %f\n",100*(hash_table->number_of_entries - num_of_heads)/(double)hash_table->number_of_entries);
    hash_table_info(hash_table);
#endif
    fclose(fp);
#if !_DEBUG_HASH_
    // find all similar words
    for (i = 0u; i < hash_table->hash_table_size; i++)
      for (node = hash_table->heads[i]; node != NULL; node = node->next)
        similar_words(hash_table, node);
    graph_info(hash_table);
    output_adjacency_lists(hash_table);

#if _VER_REPRESENTANTES_
    for (i = 0u; i < hash_table->hash_table_size; i++)
      for (node = hash_table->heads[i]; node != NULL; node = node->next)
      {
        if(node->representative == node && node->representative->number_of_vertices > 10 && node->representative->number_of_vertices < 20)
        {
          printf(">>> CC - Representante [%s]: %u\n",node->word, node->number_of_vertices);
        }
      }
    printf("Nº Comp. Conexos ----> %u\n",hash_table->n_connected_components);
#endif

    // ask what to do
    for (;;)
    {
      fprintf(stderr, "Your wish is my command:\n");
      fprintf(stderr, "  1 WORD       (list the connected component WORD belongs to)\n");
      fprintf(stderr, "  2 FROM TO    (list the shortest path from FROM to TO)\n");
      fprintf(stderr, "  3            (terminate)\n");
      fprintf(stderr, "  4 WORD       (listar representante do componente conexo a que WORD pertence)\n");
      fprintf(stderr, "  5 WORD       (listar vizinhos de WORD)\n");
      fprintf(stderr, "  6 WORD       (listar diâmetro de componente conexo ao qual WORD pertence)\n");
      fprintf(stderr, "> ");
      if (scanf("%99s", word) != 1)
        break;
      command = atoi(word);
      if (command == 1)
      {
        if (scanf("%99s", word) != 1)
          break;
        list_connected_component(hash_table, word);
      }
      else if (command == 2)
      {
        if (scanf("%99s", from) != 1)
          break;
        if (scanf("%99s", to) != 1)
          break;
        path_finder(hash_table, from, to);
      }
      else if (command == 3)
        break;
      else if (command == 4)
      {
        if (scanf("%99s", from) != 1)
          break;
        hash_table_node_t* h = find_word(hash_table, from, 0);
        if(h == NULL)
        {
          printf(">>> %s não existe na tabela de dispersão\n", from);
          continue;
        } 
        printf("R: %s\n", find_representative(h)->word);
      }
      else if (command == 5)
      {
        if (scanf("%99s", from) != 1)
          break;
        printf(">> Checking %s \n", from);
        hash_table_node_t* h = find_word(hash_table, from, 0);
        if(h == NULL)
        {
          printf(">>> %s não existe na tabela de dispersão\n", from);
          continue;
        }
        unsigned int cou = 0;
        adjacency_node_t* n;
        for(n = h->head; n != NULL; n = n->next)
        {
          printf("%d: [%s]\n",cou, n->vertex->word);
          cou++;
        }
      }
      else if (command == 6)
      {
        if (scanf("%99s", word) != 1)
          break;
        hash_table_node_t* h = find_word(hash_table, word, 0);
        if(h == NULL)
        {
          printf(">>> %s não existe na tabela de dispersão\n", h->word);
          continue;
        }
        int d = connected_component_diameter(h, 1);
        printf(">>> Diâmetro do componente conexo '%s': %d\n", find_representative(h)->word, d);
      }
    }
    // clean up
#endif
    hash_table_free(hash_table);
    return 0;
  }
