#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Coins {
  int quantity;
  char unit[4];
};

struct Equipment {
  char name[64];
  float weight;
  struct Coins coins;
  struct Equipment *next; // Pointer to the next equipment item
};

struct Inventory {
  double Maxweight;
  struct Coins coins[5]; // Array of coins (cp, sp, ep, gp, pp)
  struct Equipment
      *head; // Pointer to the first equipment item in the linked list
  int count;
};

void printUsage(void);
int Equipmentfiles(char *fileName, struct Inventory *inventory, int quantity);
void jsonParser(FILE *filePointer, struct Equipment *equipment);
void addEquipment(struct Inventory *inventory, struct Equipment *equipment);
void printInventory(struct Inventory *inventory);
void freeInventory(struct Inventory *inventory);

int main(int argc, char *argv[]) {
  char campFile[64] = "";
  struct Inventory inventory;

  // Initialize inventory
  inventory.Maxweight = 0.0;
  for (int i = 0; i < 5; i++) {
    inventory.coins[i].quantity = 0;
    inventory.coins[i].unit[0] = '\0';
  }
  inventory.head = NULL;
  inventory.count = 0;

  if (argc <= 2) {
    printUsage();
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    // Weight
    if (strcmp(argv[i], "-w") == 0) {
      inventory.Maxweight = atof(argv[++i]);
      printf("Maximum weight: %.2f\n", inventory.Maxweight);
    }
    // Money
    else if (strcmp(argv[i], "-m") == 0) {
      int j = 0;
      while (j < 5 && i + 1 < argc) {
        // Number check 0 to 9
        if (argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9') {
          sscanf(argv[++i], "%d%3s", &inventory.coins[j].quantity,
                 inventory.coins[j].unit);
          printf("coins\n%d%s\n", inventory.coins[j].quantity,
                 inventory.coins[j].unit);
        } else {
          break;
        }
        j++;
      }
    }
    // Campfile
    else if (strcmp(argv[i], "-c") == 0) {
      if (i + 1 < argc) {
        strcpy(campFile, argv[++i]);
        FILE *campFilePtr = fopen(campFile, "a");
        if (campFilePtr == NULL) {
          printf("Error opening file: %s\n", campFile);
          continue;
        }
        printf("Campfile opened: %s\n", campFile);
        fclose(campFilePtr);
      }
    }
    // Handling Equipment files
    else {
      int quantityEquipment = 1;
      if (i + 1 < argc && argv[i + 1][0] >= '0' && argv[i + 1][0] <= '9') {
        quantityEquipment = atoi(argv[++i]);
      }
      for (int j = 0; j < quantityEquipment; j++) {
        Equipmentfiles(argv[i], &inventory, quantityEquipment);
      }
    }
  }
  // Print inventory
  printInventory(&inventory);

  // Free memory
  freeInventory(&inventory);

  return 0;
}

void printUsage(void) {
  printf("Usage:   Inventory.exe equipment-files [number-of-items] [-w "
         "max-weight] [-m money] [-c camp-file]\n");
  printf("Options:\n");
  printf("    number-of-items      Optional per file to define the number in "
         "the inventory\n");
  printf(
      "    -w max-weight        Maximum weight before becoming encumbered\n");
  printf("    -m money             List of coins and types (cp, sp, ep, gp, "
         "pp)\n");
  printf("    -c camp-file         Optional camp file for all discovered items "
         "during play that stay in camp\n");
}

int Equipmentfiles(char *fileName, struct Inventory *inventory, int quantity) {
  char filePath[256];
  snprintf(filePath, sizeof(filePath), "Equipment/%s", fileName);
  FILE *filePointer = fopen(filePath, "r");
  if (filePointer == NULL) {
    printf("Error opening JSON file: %s\n", filePath);
    return 1;
  }

  struct Equipment equipment;
  jsonParser(filePointer, &equipment);

  for (int i = 0; i < quantity; i++) {
    addEquipment(inventory, &equipment);
  }

  fclose(filePointer);
  return 0;
}

void jsonParser(FILE *filePointer, struct Equipment *equipment) {
  char buffer[1024];
  char *token;
  char *parsing;
  int nameFound = 0;

  while (fgets(buffer, sizeof(buffer), filePointer) != NULL) {
    parsing = buffer;
    while ((token = strsep(&parsing, "\"")) != NULL) {
      // name of equipment
      if (!nameFound && strcmp(token, "name") == 0) {
        token = strsep(&parsing, "\"");
        token = strsep(&parsing, "\"");
        strncpy(equipment->name, token, sizeof(equipment->name) - 1);
        equipment->name[sizeof(equipment->name) - 1] = '\0';
        nameFound = 1;
        printf("Equipment name: %s\n", equipment->name);
        // Weight of equipment
      } else if (strcmp(token, "weight") == 0) {
        token = strsep(&parsing, ":");
        token = strsep(&parsing, ",");
        equipment->weight = atof(token);
        printf("Equipment weight: %.2f\n", equipment->weight);
        // quantity of cost
      } else if (strcmp(token, "quantity") == 0) {
        token = strsep(&parsing, ":");
        token = strsep(&parsing, ",");
        equipment->coins.quantity = atoi(token);
        printf("Cost quantity: %d\n", equipment->coins.quantity);
        // unit of cost
      } else if (strcmp(token, "unit") == 0) {
        token = strsep(&parsing, "\"");
        token = strsep(&parsing, "\"");
        strcpy(equipment->coins.unit, token);
        printf("Cost unit: %s\n", equipment->coins.unit);
      }
    }
  }
}

void addEquipment(struct Inventory *inventory, struct Equipment *equipment) {
  // Create a new equipment node
  struct Equipment *newEquipment =
      (struct Equipment *)malloc(sizeof(struct Equipment));
  if (newEquipment == NULL) {
    printf("Memory allocation failed.\n");
    return;
  }
  // Copy data to the new equipment node
  strcpy(newEquipment->name, equipment->name);
  newEquipment->weight = equipment->weight;
  newEquipment->coins.quantity = equipment->coins.quantity;
  strcpy(newEquipment->coins.unit, equipment->coins.unit);
  newEquipment->next = NULL;

  // Add new equipment to the end of the linked list
  if (inventory->head == NULL) {
    // If inventory is empty, set the new equipment as the head
    inventory->head = newEquipment;
  } else {
    // Traverse the linked list to find the last node
    struct Equipment *current = inventory->head;
    while (current->next != NULL) {
      current = current->next;
    }
    // Append the new equipment to the last node
    current->next = newEquipment;
  }
  inventory->count++;
}

void printInventory(struct Inventory *inventory) {
  printf("\nInventory:\n");
  printf("Maximum weight: %.2f\n", inventory->Maxweight);
  printf("Total items: %d\n", inventory->count);
  printf("Coins:\n");
  for (int i = 0; i < 5; i++) {
    if (inventory->coins[i].quantity > 0) {
      printf("%d%s\n", inventory->coins[i].quantity, inventory->coins[i].unit);
    }
  }
  printf("Equipment:\n");
  // Traverse the linked list and print each equipment item
  struct Equipment *current = inventory->head;
  while (current != NULL) {
    printf("Name: %s, Weight: %.2f, Quantity: %d, Unit: %s\n", current->name,
           current->weight, current->coins.quantity, current->coins.unit);
    current = current->next;
  }
}

void freeInventory(struct Inventory *inventory) {
  // Traverse the linked list and free memory allocated for each equipment item
  struct Equipment *current = inventory->head;
  while (current != NULL) {
    struct Equipment *temp = current;
    current = current->next;
    free(temp);
  }
}
