#pragma once
#include "qhc.h"
#include <stdlib.h>
#include <vector>
#include <math.h>

#define SHIFTL(X,m) X<<(2*m)

// Performance counters for encoding operations
int en_update_number = 0;  // Number of memory state updates during encoding
int en_coding_number = 0;  // Number of bit encoding operations

// Performance counters for decoding operations
int de_update_number = 0;  // Number of memory state updates during decoding
int de_coding_number = 0;  // Number of bit decoding operations


#define getOneBitByPos(X,bit) ((X>>bit) & 0x01)
#define getTwoBitByPos(X,i) (X>>(i-1)& 3UL) 

// Memory array for storing Hilbert curve state transitions
// Stores state information between neighboring points for optimization
char memory[32] = { 0 };

// Coordinate to Hilbert code Mapping (CHM)
// Dimensions: [current_state][x_bit][y_bit] -> encoding Hilbert code
char CHM[4][2][2] = { 0,1,3,2,0,3,1,2,2,3,1,0,2,1,3,0 };

// Coorinate to Hilbert state Mapping (CSM)
// Dimensions: [current_state][x_bit][y_bit] -> next_state
char CSM[4][2][2] = { 1,0,3,0,0,2,1,1,2,1,2,3,3,3,0,2 };

// Hilbert code to coordinate Mapping (HCM)
// Dimensions: [current_state][quadrant_code] -> (x_bit, y_bit)
char HCM[4][4] ={0,1,3,2,0,2,3,1,3,2,0,1,3,1,0,2};

// Hilbert code to next state Mapping (HSM)
// Dimensions: [current_state][quadrant_code] -> next_state
// Determines the next orientation state during decoding
char HSM[4][4] ={1,0,0,3,0,1,1,2,3,2,2,1,2,3,3,0};

/**
 * Find the index of the most significant bit (MSB) in a 32-bit value
 * 
 * @param n: 32-bit halfmask value to analyze
 * @return: Index (0-31) of the highest set bit, or -1 if n is 0
 * 
 * Uses binary search approach: checks progressively smaller bit ranges
 * to efficiently locate the MSB position in O(log n) operations
 */
int msb32_idx(halfmask_t n)
{
	int b = 0;
	if (!n) return -1;  // Return -1 for zero input
	
	// Binary search for MSB: check if bits exist in upper half, then subdivide
#define step(x) if (n >= ((halfmask_t)1) << x) b += x, n >>= x
	step(16); // Check upper 16 bits
	step(8);  // Check upper 8 bits of remaining range
	step(4);  // Check upper 4 bits of remaining range
	step(2);  // Check upper 2 bits of remaining range
	step(1);  // Check final bit
#undef step
	return b;
}

/**
 * Find the index of the most significant bit (MSB) in a 64-bit value
 * 
 * @param n: 64-bit bitmask value to analyze
 * @return: Index (0-63) of the highest set bit, or -1 if n is 0
 * 
 * Similar to msb32_idx but operates on 64-bit values
 * Used to determine the level at which codes diverge
 */
int msb64_idx(bitmask_t n)
{
	int b = 0;
	if (!n) return -1;  // Return -1 for zero input
	
	// Binary search for MSB across 64-bit range
#define step(x) if (n >= ((uint64_t)1) << x) b += x, n >>= x
	step(32); // Check upper 32 bits
	step(16); // Check upper 16 bits of remaining range
	step(8);  // Check upper 8 bits of remaining range
	step(4);  // Check upper 4 bits of remaining range
	step(2);  // Check upper 2 bits of remaining range
	step(1);  // Check final bit
#undef step
	return b;
}

/**
 * Neighbor-Aware Hilbert Curve Encoding (Final Version)
 * 
 * This function implements an optimized Hilbert curve encoding that exploits
 * spatial locality between consecutive points. By caching state information
 * from the previous point, it can skip redundant computations when encoding
 * neighboring grid cells.
 * 
 * Algorithm Overview:
 * 1. Determine how many levels can be reused from previous encoding
 * 2. Load cached state from memory array
 * 3. Iterate from reusable position to finest level
 * 4. Update state cache for future points
 * 5. Combine new bits with reusable high-order bits from previous code
 * 
 * @param prevCode: Reference to previous point's Hilbert code (input/output)
 *                  High-order bits may be reused if coordinates are similar
 * @param iterStartPos: Reference to iteration start position (input/output)
 *                      Indicates which bit level to begin computation from
 * @param GridX: Current point's X coordinate in grid space
 * @param GridY: Current point's Y coordinate in grid space
 * @param nextX: Next point's X coordinate (used for lookahead optimization)
 * @param nextY: Next point's Y coordinate (used for lookahead optimization)
 * @param k: Maximum depth/order of the Hilbert curve (number of bit levels)
 * 
 * @return: The encoded Hilbert curve value for the current point
 * 
 * Key Optimization:
 * - If current and next points share high-order bits, those bits can be
 *   cached for the next encoding operation, reducing computational cost
 * - Memory array stores intermediate states to avoid recalculating
 *   state transitions for shared bit prefixes
 */
bitmask_t en_neibourAware(bitmask_t &prevCode, int &iterStartPos, halfmask_t GridX, halfmask_t GridY, halfmask_t nextX, halfmask_t nextY, int k)
{
	char nType = 0;         // Current Hilbert curve state (orientation/rotation)
	int startPos = 0;       // Starting position for iteration (unused in current implementation)
	int newIterStartPos = iterStartPos;  // New start position for next point
	unsigned bitX = 0;      // Current bit from X coordinate
	unsigned bitY = 0;      // Current bit from Y coordinate

	bitmask_t resKey = 0;   // Result: encoded Hilbert curve value

	// Calculate the new iteration start position for the next point
	// This determines how many high-order bits can be reused
	// The start position is where the next point's coordinates begin to differ
	// from the current point's coordinates
	
	// XOR reveals differing bits; MAX handles case where both differ
	// Higher MSB index means more shared high-order bits
	newIterStartPos = msb32_idx(MAX((nextX ^ GridX),( nextY ^ GridY)));
	
	// Load the cached state from the previous point
	// memory[k - iterStartPos - 1] stores the state at level iterStartPos
	nType = memory[k - iterStartPos - 1];

	// Main encoding loop: iterate from the start position down to level 0
	// Each iteration processes one bit level of the quadtree
	for (int i = iterStartPos; i >= 0; i--)
	{
		en_coding_number++;  // Increment encoding operation counter
		
		// Extract the i-th bit from X and Y coordinates
		bitX = getOneBitByPos(GridX, i);
		bitY = getOneBitByPos(GridY, i);
		
		// Perform state transition:
		// 1. Look up quadrant code using current state and coordinate bits
		// 2. Shift result left by 2 bits and OR with previous bits
		resKey = (resKey << 2) | CHM[nType][bitX][bitY];
		
		// Update state for next iteration using type transition matrix
		nType = CSM[nType][bitX][bitY];
		
		// Cache intermediate states for future use
		// Only update states that will be useful for the next point
		// i > newIterStartPos means this level will be recomputed for next point, so we store its state in memory for reuse
		if (i>newIterStartPos)
		{ 
			en_update_number++;  // Increment state update counter
			memory[k - i] = nType;  // Store state at this level
		}
	}

	// Combine the newly computed low-order bits with reusable high-order bits
	// from the previous code:
	// 1. Right shift prevCode to remove bits that were recomputed
	// 2. Left shift back to restore position, zeroing out low-order bits
	// 3. OR with resKey to combine reused and new bits
	int movedbits = 2 * (iterStartPos + 1);  // Number of bits recomputed
	resKey |= prevCode >> movedbits << movedbits;
	
	// Update the iteration start position for the next encoding operation
	iterStartPos = newIterStartPos;
	
	// Store the current code as the previous code for next iteration
	prevCode = resKey;

	return resKey;
}


/**
 * Neighbor-Aware Hilbert Curve Decoding (Final Version)
 * 
 * This function decodes a Hilbert curve value back to 2D grid coordinates (X,Y).
 * Like the encoding function, it exploits spatial locality by caching state
 * information and reusing computations from the previous point.
 * 
 * Algorithm Overview:
 * 1. Determine how many levels differ between current and next codes
 * 2. Load cached state from memory array
 * 3. Iterate from reusable position to finest level
 * 4. Extract coordinate bits using inverse state transition matrices
 * 5. Update state cache for future points
 * 6. Combine new coordinate bits with reusable high-order bits
 * 
 * @param prevLon: Reference to previous point's X coordinate (input/output)
 *                 Used to reuse high-order bits for current point
 * @param prevLat: Reference to previous point's Y coordinate (input/output)
 *                 Used to reuse high-order bits for current point
 * @param iterStartPos: Reference to iteration start position (input/output)
 *                      Indicates which bit level to begin computation from
 * @param currentLon: Reference to output current point's X coordinate
 * @param currentLat: Reference to output current point's Y coordinate
 * @param currentCode: Current point's Hilbert curve code to decode
 * @param nextCode: Next point's Hilbert curve code (for lookahead optimization)
 * @param k: Maximum depth/order of the Hilbert curve (number of bit levels)
 * 
 * @return: void (results returned via reference parameters currentLon, currentLat)
 * 
 * Key Optimization:
 * - Compares current and next codes to determine shared bit prefixes
 * - Reuses high-order coordinate bits from previous point
 * - Caches intermediate states to minimize redundant computations
 */
void de_neibourAware(halfmask_t &prevLon, halfmask_t &prevLat, int &iterStartPos, 
	halfmask_t &currentLon, halfmask_t &currentLat, bitmask_t currentCode, bitmask_t nextCode, int k)
	
	char nType = 0;         // Current Hilbert curve state (orientation/rotation)
	char posKey = 0;        // Decoded position key (contains x,y bit pair)

	currentLon = 0;         // Initialize current X coordinate
	currentLat = 0;         // Initialize current Y coordinate
	int startPos = 0;       // Starting position (unused in current implementation)
	int newIterStartPos = iterStartPos;  // New start position for next decoding
	unsigned bitsZ = 0;     // Two-bit quadrant code extracted from Hilbert curve

	// Calculate the new iteration start position for the next decoding operation
	// XOR of codes reveals where they differ; MSB of difference indicates
	// the highest level at which codes diverge
	// Divide by 2 because Hilbert codes use 2 bits per level (4 quadrants)
	
	newIterStartPos = msb64_idx(currentCode ^ nextCode) / 2;
	
	// Load the cached state from the previous point's decoding
	// This allows resuming computation from the level where recomputation is needed
	nType = memory[k - iterStartPos - 1];

	// Main decoding loop: iterate from start position down to level 0
	// Split into two conceptual segments to reduce lookup operations:
	// 1. Segment requiring state updates (i > newIterStartPos)
	// 2. Segment without state updates (i <= newIterStartPos)
	for (int i = iterStartPos; i >= 0; i--)
	{
		de_coding_number++;  // Increment decoding operation counter
		
		// Extract 2-bit quadrant code from current position in Hilbert curve
		// (2*i+1) gets the two bits at level i
		bitsZ = getTwoBitByPos(currentCode, (2 * i + 1));
		
		// Inverse transformation: map quadrant code back to coordinate bits
		// using inverse key matrix based on current state
		posKey = HCM[nType][bitsZ];
		
		// Extract and append coordinate bits:
		// Bit 0 of posKey is the Y coordinate bit
		// Bit 1 of posKey is the X coordinate bit
		currentLat = (currentLat << 1) | (posKey & 0x1);       // Y coordinate bit
		currentLon = (currentLon << 1) | (posKey >> 1 & 0x1);  // X coordinate bit
		
		// Update state for next iteration using inverse type transition matrix
		nType = HSM[nType][bitsZ];
		
		// Cache intermediate states for future use
		// Only update states that will be useful for the next decoding
		// i > newIterStartPos means this level will be recomputed for next code,
		// so we store its state in memory for reuse
		if (i > newIterStartPos)
		{
			de_update_number++;  // Increment state update counter
			memory[k - i] = nType;  // Store state at this level
		}
	}

	// Combine the newly decoded low-order bits with reusable high-order bits
	// from the previous coordinates:
	// 1. Right shift previous coordinates to keep only reusable high-order bits
	// 2. Left shift back to restore position, effectively zeroing low-order bits
	// 3. OR with current coordinates to combine reused and new bits
	int movedbits = (iterStartPos + 1);  // Number of bit levels recomputed
	
	currentLon = currentLon | (prevLon >> movedbits << movedbits);
	currentLat = currentLat | (prevLat >> movedbits << movedbits);

	// Update the iteration start position for the next decoding operation
	iterStartPos = newIterStartPos;
	
	// Store current coordinates as previous for next iteration
	prevLat = currentLat;
	prevLon = currentLon;
}
