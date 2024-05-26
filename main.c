#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Coins {
  int quantity;
  char unit[3];
};

struct Equipment {
  char name[64];
  int itemCount;
  double weight;
  struct Coins coins;
  struct Equipment *next;
  struct Equipment *prev;
};

struct Inventory {
  double maxWeight;
  double currentWeight;
  struct Coins coins[5];
  struct Equipment *equipment;
  char *campFile;
};

void printUsage(void);
int isValidCoin(const char *unit);
int EquipmentFile(char *fileName, struct Inventory *inventory, int itemCount);
void jsonParser(FILE *filePointer, struct Equipment *equipment);
void addEquipment(struct Inventory *inventory, struct Equipment *newEquipment,
                  int itemCount);
void printInventory(struct Inventory *inventory);
void freeInventory(struct Inventory *inventory);
void manageInventory(struct Inventory *inventory);

int main(int argc, char *argv[]) {
  struct Inventory inventory = {0};
  int coinIndex = 0;

  if (argc < 2) {
    printUsage();
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    // Weight
    if (strcmp(argv[i], "-w") == 0) {
      if (i + 1 < argc) {
        inventory.maxWeight = atof(argv[++i]);
        printf("Set maximum weight: %.2f\n", inventory.maxWeight);
      }
    }
    // Coins
    else if (strcmp(argv[i], "-m") == 0) {
      while (i + 1 < argc && isdigit(argv[i + 1][0])) {
        int qty;
        char unit[3];
        if (sscanf(argv[++i], "%d%2s", &qty, unit) == 2 && isValidCoin(unit)) {
          inventory.coins[coinIndex].quantity = qty;
          strcpy(inventory.coins[coinIndex].unit, unit);
          printf("Coins: %d %s\n", inventory.coins[coinIndex].quantity,
                 inventory.coins[coinIndex].unit);
          coinIndex++;
        } else {
          break;
        }
      }
    }
    // creating campfile
    else if (strcmp(argv[i], "-c") == 0) {
      if (i + 1 < argc) {
        inventory.campFile = argv[++i];
        printf("Camp log option detected: %s\n", inventory.campFile);
        FILE *file = fopen(inventory.campFile, "a");
        if (file != NULL) {
          fclose(file);
          printf("Camp log file created: %s\n", inventory.campFile);
        } else {
          printf("Error: Unable to create camp log file: %s\n",
                 inventory.campFile);
        }
      }
    } else {
      int itemCount = 1;
      char *filename = argv[i];
      if (i + 1 < argc && isdigit(argv[i + 1][0])) {
        itemCount = atoi(argv[++i]);
      }
      EquipmentFile(filename, &inventory, itemCount);
    }
  }

  printInventory(&inventory);
  manageInventory(&inventory);
  freeInventory(&inventory);

  return 0;
}

void printUsage(void) {
  printf("Usage: Inventory.exe equipment-files [number-of-items] [-w "
         "max-weight] [-m money]\n");
  printf("Options:\n");
  printf("    number-of-items      Optional per file to define the number in "
         "the inventory\n");
  printf(
      "    -w max-weight        Maximum weight before becoming encumbered\n");
  printf("    -m money             List of coins and types (cp, sp, ep, gp, "
         "pp)\n");
}

int isValidCoin(const char *unit) {
  // validate coins
  return strcmp(unit, "cp") == 0 || strcmp(unit, "sp") == 0 ||
         strcmp(unit, "ep") == 0 || strcmp(unit, "gp") == 0 ||
         strcmp(unit, "pp") == 0;
}

int EquipmentFile(char *fileName, struct Inventory *inventory, int itemCount) {
  // Calculate file path
  char *filePath = malloc(strlen("Equipment/") + strlen(fileName) + 1);
  if (filePath == NULL) {
    printf("Memory allocation failed.\n");
    return 1;
  }

  strcpy(filePath, "Equipment/");
  strcat(filePath, fileName);

  // Reading JSON files
  FILE *filePointer = fopen(filePath, "r");
  if (filePointer == NULL) {
    printf("Error opening JSON file: %s\n", filePath);
    free(filePath);
    return 1;
  }

  struct Equipment *equipment = malloc(sizeof(struct Equipment));
  if (equipment == NULL) {
    printf("Memory allocation failed.\n");
    fclose(filePointer);
    free(filePath);
    return 1;
  }

  memset(equipment, 0, sizeof(struct Equipment));
  jsonParser(filePointer, equipment);
  addEquipment(inventory, equipment, itemCount);

  fclose(filePointer);
  free(filePath);
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
      // Getting name
      if (!nameFound && strcmp(token, "name") == 0) {
        token = strsep(&parsing, "\"");
        token = strsep(&parsing, "\"");
        snprintf(equipment->name, sizeof(equipment->name), "%s", token);
        nameFound = 1;
        printf("Equipment name: %s\n", equipment->name);
      }
      // Getting weight
      else if (strcmp(token, "weight") == 0) {
        token = strsep(&parsing, ":");
        token = strsep(&parsing, ",");
        equipment->weight = atof(token);
        printf("Equipment weight: %.2f\n", equipment->weight);
      } // Getting coins
      else if (strcmp(token, "quantity") == 0) {
        token = strsep(&parsing, ":");
        token = strsep(&parsing, ",");
        equipment->coins.quantity = atoi(token);
        printf("Cost quantity: %d\n", equipment->coins.quantity);
      } else if (strcmp(token, "unit") == 0) {
        token = strsep(&parsing, "\"");
        token = strsep(&parsing, "\"");
        snprintf(equipment->coins.unit, sizeof(equipment->coins.unit), "%s",
                 token);
        printf("Cost unit: %s\n", equipment->coins.unit);
      }
    }
  }
}

void addEquipment(struct Inventory *inventory, struct Equipment *newEquipment,
                  int itemCount) {
  if (newEquipment == NULL) {
    return;
  }

  struct Equipment *copiedEquipment = malloc(sizeof(struct Equipment));
  if (copiedEquipment == NULL) {
    printf("Memory allocation failed.\n");
    return;
  }
  memcpy(copiedEquipment, newEquipment, sizeof(struct Equipment));
  copiedEquipment->itemCount = itemCount;

  // Check if adding this equipment exceeds the maximum weight
  double newWeight = copiedEquipment->weight * itemCount;
  if (inventory->currentWeight + newWeight > inventory->maxWeight) {
    printf("Player is encumbered. Cannot add equipment '%s'.\n",
           copiedEquipment->name);
    free(copiedEquipment);
    return;
  }

  // Add the equipment to the inventory
  if (inventory->equipment == NULL) {
    inventory->equipment = copiedEquipment;
    copiedEquipment->next = copiedEquipment;
    copiedEquipment->prev = copiedEquipment;
  } else {
    struct Equipment *lastEquipment = inventory->equipment->prev;
    lastEquipment->next = copiedEquipment;
    copiedEquipment->prev = lastEquipment;
    copiedEquipment->next = inventory->equipment;
    inventory->equipment->prev = copiedEquipment;
  }

  // Update the total weight
  inventory->currentWeight += newWeight;
}

void freeInventory(struct Inventory *inventory) {
  // Free equipment
  struct Equipment *current = inventory->equipment;
  while (current != NULL) {
    struct Equipment *next = current->next;
    free(current);
    current = next;
  }
}

void printInventory(struct Inventory *inventory) {
  printf("\nPlayer's Inventory:\n");

  // Print coins
  printf("Coins:\n");
  for (int i = 0; i < 5; i++) {
    printf("%d %s\n", inventory->coins[i].quantity, inventory->coins[i].unit);
  }

  // Print equipment
  printf("Equipment:\n");
  struct Equipment *current = inventory->equipment;
  double totalWeight = 0.0;
  if (current == NULL) {
    printf("No equipment\n");
  } else {
    do {
      printf("Name: %s, Weight: %.2f, Item Count: %d, Cost: %d %s\n",
             current->name, current->weight, current->itemCount,
             current->coins.quantity, current->coins.unit);
      totalWeight += current->weight * current->itemCount;
      current = current->next;
    } while (current != inventory->equipment);
  }
  printf("Total Weight: %.2f\n", totalWeight);
}

void manageInventory(struct Inventory *inventory) {
  int option;
  struct Equipment *currentEquipment = inventory->equipment;

  while (1) {
    printf("\nInventory Management:\n");
    printf("1. Transfer equipment from player to camp\n");
    printf("2. Transfer equipment from camp to player\n");
    printf("3. Check next equipment in player inventory\n");
    printf("4. Check previous equipment in player inventory\n");
    printf("5. List camp's equipment\n");
    printf("6. Exit inventory\n");
    printf("Choose an option: ");

    int result = scanf("%d", &option);
    if (result != 1 || option < 1 || option > 6) {
      printf("Invalid input. Please enter a number between 1 and 6.\n");
      while (getchar() != '\n')
        continue;
    }

    switch (option) {
    case 1:
      // Transfer equipment from player to camp
      if (inventory->campFile == NULL) {
        printf("There is no campfile.\n");
        break;
      }

      if (currentEquipment != NULL) {
        struct Equipment *transferEquipment = currentEquipment;
        // Update player's equipment list
        if (currentEquipment->prev == currentEquipment) {
          inventory->equipment = NULL;
        } else {
          currentEquipment->prev->next = currentEquipment->next;
          currentEquipment->next->prev = currentEquipment->prev;
          inventory->equipment = currentEquipment->next;
          currentEquipment = currentEquipment->next;
        }
        printf("Transferring equipment '%s' from player to camp...\n",
               transferEquipment->name);
        // Update total weight
        inventory->currentWeight -=
            (transferEquipment->weight * transferEquipment->itemCount);
        printf("TotalWeight: %.2f\n", inventory->currentWeight);
        // Write equipment transfer details to camp's inventory file
        FILE *campFile = fopen(inventory->campFile, "a");
        if (campFile != NULL) {
          fprintf(campFile, "Name:%s Item count:%d Weight:%.2f Cost:%d %s\n",
                  transferEquipment->name, transferEquipment->itemCount,
                  transferEquipment->weight, transferEquipment->coins.quantity,
                  transferEquipment->coins.unit);
          fclose(campFile);
          printf("Equipment transferred successfully.\n");
        } else {
          printf("Error: Unable to open camp log file for writing.\n");
        }
        free(transferEquipment);
      } else {
        printf("No more equipment in player inventory.\n");
      }
      break;
    case 2:
      // Transfer equipment from camp to player
      printf("Transferring equipment from camp to player...\n");
      FILE *campFileRead = fopen(inventory->campFile, "r");
      if (campFileRead != NULL) {
        char name[64], unit[3];
        int itemCount;
        double weight;
        int quantity;
        while (fscanf(campFileRead,
                      " Name:%63s Item count:%d Weight:%lf Cost:%d %2s\n", name,
                      &itemCount, &weight, &quantity, unit) == 5) {
          struct Equipment *newEquipment = malloc(sizeof(struct Equipment));
          if (newEquipment != NULL) {
            strcpy(newEquipment->name, name);
            newEquipment->itemCount = itemCount;
            newEquipment->weight = weight;
            newEquipment->coins.quantity = quantity;
            strcpy(newEquipment->coins.unit, unit);

            // Update total weight
            inventory->currentWeight +=
                (newEquipment->weight * newEquipment->itemCount);
            printf("TotalWeight: %.2f\n", inventory->currentWeight);

            // Add equipment to player's inventory
            if (inventory->equipment == NULL) {
              inventory->equipment = newEquipment;
              newEquipment->next = newEquipment;
              newEquipment->prev = newEquipment;
            } else {
              struct Equipment *lastEquipment = inventory->equipment->prev;
              lastEquipment->next = newEquipment;
              newEquipment->prev = lastEquipment;
              newEquipment->next = inventory->equipment;
              inventory->equipment->prev = newEquipment;
            }
          } else {
            printf("Memory allocation failed. Equipment transfer aborted.\n");
            // Free previously allocated memory
            struct Equipment *current = inventory->equipment;
            while (current != NULL) {
              struct Equipment *next = current->next;
              free(current);
              current = next;
            }
            fclose(campFileRead);
            break;
          }
        }
        fclose(campFileRead);
        // Clear camp's inventory file after transfer
        FILE *campFileClear = fopen(inventory->campFile, "w");
        if (campFileClear != NULL) {
          fclose(campFileClear);
          printf("Equipment transferred successfully.\n");
        } else {
          printf("Error: Unable to clear camp log file after transfer.\n");
        }
      } else {
        printf("Error: Unable to open camp log file for reading.\n");
      }
      break;
    case 3:
      // Move to next equipment
      if (currentEquipment != NULL) {
        currentEquipment = currentEquipment->next;
        printf("Next equipment: Name: %s, Weight: %.2f, Item Count: %d, Cost: "
               "%d %s\n",
               currentEquipment->name, currentEquipment->weight,
               currentEquipment->itemCount, currentEquipment->coins.quantity,
               currentEquipment->coins.unit);
      } else {
        printf("No more equipment in player inventory.\n");
      }
      break;
    case 4:
      // Move to previous equipment
      if (currentEquipment != NULL) {
        currentEquipment = currentEquipment->prev;
        printf("Previous equipment: Name: %s, Weight: %.2f, Item Count: %d, "
               "Cost: %d %s\n",
               currentEquipment->name, currentEquipment->weight,
               currentEquipment->itemCount, currentEquipment->coins.quantity,
               currentEquipment->coins.unit);
      } else {
        printf("No previous equipment in player inventory.\n");
      }
      break;
    case 5:
      // List camp's equipment
      printf("Camp's Equipment:\n");
      FILE *campFileList = fopen(inventory->campFile, "r");
      if (campFileList != NULL) {
        char name[64], unit[3];
        int itemCount;
        double weight;
        int quantity;
        while (fscanf(campFileList,
                      " Name:%63s Item count:%d Weight:%lf Cost:%d %2s\n", name,
                      &itemCount, &weight, &quantity, unit) == 5) {
          printf("Name: %s, Weight: %.2f, Item Count: %d, Cost: %d %s\n", name,
                 weight, itemCount, quantity, unit);
        }
        fclose(campFileList);
      } else {
        printf("Error: Unable to open camp log file for reading.\n");
      }
      break;
    case 6:
      printf("Exiting inventory management.\n");
      exit(0);
    default:
      printf("Invalid option. Please choose a valid option.\n");
      break;
    }
  }
}
